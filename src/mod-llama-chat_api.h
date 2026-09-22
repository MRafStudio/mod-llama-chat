#ifndef MOD_LLAMA_API_H
#define MOD_LLAMA_API_H

#include "mod-llama-chat_capability.h"
#include <string>
#include <cstdint>
#include <string>

// Result of one generation call. The old API returned a bare string, so
// "server unreachable", "model refused to think" and "model said nothing"
// were indistinguishable -- all three surfaced as an empty reply and a
// generic log line.
struct LlamaApiResult
{
    std::string text;             // model output, reasoning already removed
    std::string thinking;         // native reasoning, kept only for debug logs
    bool        ok        = false;
    int         status    = 0;
    std::string error;
    uint64_t    latencyMs = 0;
    bool        thinkUsed = false;

    bool empty() const { return text.empty(); }
};

// --------------------------------------------------------------------------
// Immutable snapshot of everything a generation call needs from config.
//
// Worker threads used to read g_LlamaUrl / g_LlamaModel / g_LlamaSystemPrompt
// / g_LlamaStop / g_LlamaSeed directly. `.llama reload` reassigns those
// std::strings on the world thread, which frees the old buffer underneath any
// worker mid-copy -- an access violation deep inside the HTTP client, with a
// stack that points at httplib rather than at the real cause.
//
// Config is now published once under a mutex and workers take a copy.
// --------------------------------------------------------------------------
struct LlamaEndpointSettings
{
    std::string url;
    std::string model;
    std::string systemPrompt;
    std::string stop;
    std::string seed;

    uint32_t numPredict = 0;
    uint32_t numCtx     = 0;
    uint32_t numThreads = 0;

    float temperature   = 0.8f;
    float topP          = 0.95f;
    float repeatPenalty = 1.1f;

    // Optional sampling controls. Negative means "unset" -- the field is then
    // omitted from the request entirely and the model's own default applies.
    int32_t topK             = -1;
    float   minP             = -1.0f;
    float   presencePenalty  = -1000.0f;
    float   frequencyPenalty = -1000.0f;

    // --- [MRafStudio fork] протокол и авторизация -------------------------
    // "ollama" (по умолчанию) -- нативный /api/generate (+ /api/show);
    // "openai" -- OpenAI-совместимый /v1/chat/completions (llama.cpp, vLLM,
    //             Llama >=0.2, DeepSeek, LM Studio, OpenRouter...).
    std::string apiMode      = "ollama";
    // Непустой ключ уходит заголовком "Authorization: Bearer <key>".
    std::string openAiKey;
    // Reasoning-модели без этого флага уходят в "размышления" целиком:
    // content приходит пустым, ответ -- в reasoning_content.
    bool        openAiDisableThinking = true;
};

// Republish from the g_Llama* globals. Call on the world thread after config
// load or reload.
void LlamaConfig_Publish();

// Thread-safe copy for a worker.
LlamaEndpointSettings LlamaConfig_Snapshot();

// Perform one generation. Decides whether to think based on the configured
// policy and the probed model capability, and transparently retries once
// without thinking if Llama rejects the request for asking.
//
// Blocking. Call from a worker thread, never from the world thread.
LlamaApiResult QueryLlama(const std::string& prompt, LlamaRequestKind kind);

// Legacy shim: returns the text, or empty on any failure.
std::string QueryLlamaAPI(const std::string& prompt);

bool IsValidAPIResponse(const std::string& response);

#endif // MOD_LLAMA_API_H
