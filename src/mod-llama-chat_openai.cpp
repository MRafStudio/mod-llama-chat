#include "mod-llama-chat_openai.h"

#include "mod-llama-chat-utilities.h"
#include "mod-llama-chat_config.h"
#include "mod-llama-chat_httpclient.h"

#include "Log.h"
#include "nlohmann/json.hpp"

#include <chrono>
#include <mutex>
#include <sstream>
#include <vector>

// ---------------------------------------------------------------------------
//  Разбор ответа /v1/chat/completions
//    { "choices": [ { "message": { "content": "...", "reasoning_content": "..." },
//                     "finish_reason": "stop" } ], "error": {...} }
// ---------------------------------------------------------------------------
namespace
{
    // Один клиент на рабочий поток модуля (клиент httplib пулит keep-alive сокеты).
    thread_local LlamaHttpClient t_openAiClient;

    std::vector<std::string> SplitByComma(const std::string& value)
    {
        std::vector<std::string> out;
        std::stringstream ss(value);
        std::string item;
        while (std::getline(ss, item, ','))
        {
            // trim
            size_t b = item.find_first_not_of(" \t\r\n");
            size_t e = item.find_last_not_of(" \t\r\n");
            if (b != std::string::npos)
                out.push_back(item.substr(b, e - b + 1));
        }
        return out;
    }

    void ParseOpenAiBody(const std::string& body,
                         std::string& outText,
                         std::string& outThinking,
                         std::string& outError,
                         std::string& outFinishReason)
    {
        try
        {
            nlohmann::json parsed = nlohmann::json::parse(body);

            // Ошибку может вернуть и OpenAI-совместимый сервер (llama.cpp делает это
            // строкой, а не объектом) -- поддержаны оба вида.
            if (parsed.contains("error"))
            {
                const auto& err = parsed["error"];
                if (err.is_string())
                    outError = err.get<std::string>();
                else if (err.is_object() && err.contains("message"))
                    outError = err["message"].get<std::string>();
                else
                    outError = "server reported an error";
                return;
            }

            if (!parsed.contains("choices") || !parsed["choices"].is_array() || parsed["choices"].empty())
            {
                outError = "response has no choices[]";
                return;
            }

            const auto& choice = parsed["choices"][0];

            if (choice.contains("finish_reason") && choice["finish_reason"].is_string())
                outFinishReason = choice["finish_reason"].get<std::string>();

            if (choice.contains("message") && choice["message"].is_object())
            {
                const auto& msg = choice["message"];
                if (msg.contains("content") && msg["content"].is_string())
                    outText = msg["content"].get<std::string>();
                // Некоторые серверы (и reasoning-модели) отдают всё сюда.
                if (msg.contains("reasoning_content") && msg["reasoning_content"].is_string())
                    outThinking = msg["reasoning_content"].get<std::string>();
            }
            else if (choice.contains("text") && choice["text"].is_string())
            {
                // на случай /v1/completions-подобного ответа
                outText = choice["text"].get<std::string>();
            }

            // Модель ушла в рассуждения и не выдала ответ: отдаём рассуждения,
            // иначе игрок увидит пустую реплику (классика reasoning-моделей).
            if (outText.empty() && !outThinking.empty())
                outText = outThinking;
        }
        catch (const std::exception& e)
        {
            outError = std::string("JSON parse failure: ") + e.what();
        }
    }
}

namespace LlamaOpenAi
{
    bool IsEnabled(const LlamaEndpointSettings& cfg)
    {
        return cfg.apiMode == "openai" || cfg.apiMode == "openai-compatible" || cfg.apiMode == "v1";
    }

    // =======================================================================
    //  Сборщики тел для будущих ролей (задел под мультимодальность).
    //  Пока модуль-болтовня их не вызывает: он отвечает только за текст.
    //  Когда понадобятся картинки/аудио -- эти тела уже готовы и
    //  используют тот же HTTP-клиент модуля (keep-alive, таймауты, Bearer).
    // =======================================================================

    // (vision) Текст + одно изображение (data:URL или http(s) ссылка)
    nlohmann::json BuildVisionChatBody(const std::string& model,
                                       const std::string& systemPrompt,
                                       const std::string& userText,
                                       const std::string& imageUrlOrDataUri)
    {
        nlohmann::json messages = nlohmann::json::array();
        if (!systemPrompt.empty())
            messages.push_back({ { "role", "system" }, { "content", SanitizeUTF8(systemPrompt) } });

        nlohmann::json parts = nlohmann::json::array();
        parts.push_back({ { "type", "text" }, { "text", SanitizeUTF8(userText) } });
        parts.push_back({ { "type", "image_url" }, { "image_url", { { "url", imageUrlOrDataUri } } } });

        messages.push_back({ { "role", "user" }, { "content", parts } });
        return { { "model", model }, { "stream", false }, { "messages", messages } };
    }

    // (генерация изображений) POST на /v1/images/generations
    nlohmann::json BuildImageGenerationBody(const std::string& model,
                                            const std::string& prompt,
                                            const std::string& size,
                                            uint32_t count)
    {
        nlohmann::json body = {
            { "model",  model },
            { "prompt", SanitizeUTF8(prompt) },
            { "n",      count == 0 ? 1u : count },
        };
        if (!size.empty())
            body["size"] = size;
        return body;
    }

    // (распознавание речи) -- тело multipart/form-data на /v1/audio/transcriptions
    // Возвращает готовую строку тела и её content-type; fileBytes -- сырые байты аудио.
    std::string BuildAudioTranscriptionBody(const std::string& fileName,
                                            const std::string& fileBytes,
                                            const std::string& model,
                                            const std::string& language,
                                            std::string& outContentType)
    {
        const std::string boundary = "----HermesAudioBoundary7f3d";

        auto part = [&](const std::string& name, const std::string& value)
        {
            std::string s = "--" + boundary + "\r\n";
            s += "Content-Disposition: form-data; name=\"" + name + "\"\r\n\r\n";
            s += value + "\r\n";
            return s;
        };

        std::string body;
        if (!model.empty())    body += part("model", model);
        if (!language.empty()) body += part("language", language);

        body += "--" + boundary + "\r\n";
        body += "Content-Disposition: form-data; name=\"file\"; filename=\"" + fileName + "\"\r\n";
        body += "Content-Type: application/octet-stream\r\n\r\n";

        const std::string tail = "\r\n--" + boundary + "--\r\n";
        outContentType = "multipart/form-data; boundary=" + boundary;

        // Возвращаем склейку: заголовок части + байты файла + закрывающая граница.
        return body + fileBytes + tail;
    }

    // (синтез речи) POST на /v1/audio/speech
    nlohmann::json BuildSpeechBody(const std::string& model,
                                   const std::string& text,
                                   const std::string& voice,
                                   const std::string& responseFormat)
    {
        nlohmann::json body = {
            { "model", model },
            { "input", SanitizeUTF8(text) },
        };
        body["voice"]           = voice.empty() ? "alloy" : voice;
        body["response_format"] = responseFormat.empty() ? "mp3" : responseFormat;
        return body;
    }

    // =======================================================================
    //  Основной путь: одна текстовая генерация
    // =======================================================================
    LlamaApiResult PerformOnce(const LlamaEndpointSettings& cfg,
                                const std::string& prompt,
                                const std::string& /*thinkLevel*/,
                                uint32_t reasoningReserve)
    {
        LlamaApiResult result;

        // ---- тело запроса -------------------------------------------------
        nlohmann::json messages = nlohmann::json::array();
        if (!cfg.systemPrompt.empty())
            messages.push_back({ { "role", "system" }, { "content", SanitizeUTF8(cfg.systemPrompt) } });
        messages.push_back({ { "role", "user" }, { "content", SanitizeUTF8(prompt) } });

        nlohmann::json request = {
            { "model",    cfg.model },
            { "stream",   false },
            { "messages", messages },
        };

        // OpenAI-имена параметров сэмплинга.
        if (cfg.numPredict > 0)
            request["max_tokens"] = cfg.numPredict + reasoningReserve;

        request["temperature"] = cfg.temperature;
        request["top_p"]       = cfg.topP;

        // Расширения llama.cpp: имена совпадают с ollama-режимом.
        if (cfg.repeatPenalty != 1.1f)     request["repeat_penalty"]   = cfg.repeatPenalty;
        if (cfg.presencePenalty > -999.0f)  request["presence_penalty"]  = cfg.presencePenalty;
        if (cfg.frequencyPenalty > -999.0f) request["frequency_penalty"] = cfg.frequencyPenalty;
        if (cfg.topK >= 0)                  request["top_k"]             = cfg.topK;
        if (cfg.minP >= 0.0f)               request["min_p"]             = cfg.minP;

        if (!cfg.stop.empty())
            request["stop"] = SplitByComma(cfg.stop);

        if (!cfg.seed.empty())
        {
            try { request["seed"] = std::stoi(cfg.seed); }
            catch (const std::exception&) { /* неверный seed -- просто не отправляем */ }
        }

        // Reasoning-модели (Qwen3 и подобные) без этого флага целиком уходят в
        // "размышления": content приходит пустым, весь текст -- в reasoning_content.
        if (cfg.openAiDisableThinking)
            request["chat_template_kwargs"] = { { "enable_thinking", false } };

        // ---- отправка -----------------------------------------------------
        const auto started = std::chrono::steady_clock::now();
        LlamaHttpResult http = t_openAiClient.PostExWithToken(cfg.url, request.dump(), cfg.openAiKey);
        const auto finished = std::chrono::steady_clock::now();

        result.latencyMs = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(finished - started).count());
        result.status = http.status;

        if (!http.error.empty())
        {
            result.error = http.error;
            return result;
        }

        if (!http.ok())
        {
            result.error = "HTTP " + std::to_string(http.status);
            if (!http.body.empty())
                result.error += ": " + http.body.substr(0, 300);
            return result;
        }

        std::string finishReason;
        std::string parseError;
        ParseOpenAiBody(http.body, result.text, result.thinking, parseError, finishReason);

        if (!parseError.empty())
        {
            result.error = parseError;
            return result;
        }

        if (g_DebugEnabled)
            LOG_INFO("module.mod_llama_chat", "[Llama Chat] OpenAI-compatible reply ok in {}ms (finish_reason={})",
                     result.latencyMs, finishReason);

        result.ok = true;
        return result;
    }
}
