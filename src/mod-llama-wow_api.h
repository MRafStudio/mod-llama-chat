#ifndef MOD_LLAMA_WOW_API_H
#define MOD_LLAMA_WOW_API_H

#include "mod-llama-wow_capability.h"
#include <string>
#include <cstdint>
#include <string>

// Result of one generation call. The old API returned a bare string, so
// "server unreachable", "model refused to think" and "model said nothing"
// were indistinguishable -- all three surfaced as an empty reply and a
// generic log line.
struct LlamaWowApiResult
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
// Worker threads used to read g_LlamaWowUrl / g_LlamaWowModel / g_LlamaWowSystemPrompt
// / g_LlamaWowStop / g_LlamaWowSeed directly. `.llamawow reload` reassigns those
// std::strings on the world thread, which frees the old buffer underneath any
// worker mid-copy -- an access violation deep inside the HTTP client, with a
// stack that points at httplib rather than at the real cause.
//
// Config is now published once under a mutex and workers take a copy.
// --------------------------------------------------------------------------
struct LlamaWowEndpointSettings
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
    //             LlamaWow >=0.2, DeepSeek, LM Studio, OpenRouter...).
    std::string apiMode      = "ollama";
    // Непустой ключ уходит заголовком "Authorization: Bearer <key>".
    std::string openAiKey;
    // Reasoning-модели без этого флага уходят в "размышления" целиком:
    // content приходит пустым, ответ -- в reasoning_content.
    bool        openAiDisableThinking = true;
};

// Republish from the g_LlamaWow* globals. Call on the world thread after config
// load or reload.
void LlamaWowConfig_Publish();

// Thread-safe copy for a worker.
LlamaWowEndpointSettings LlamaWowConfig_Snapshot();

// Perform one generation. Decides whether to think based on the configured
// policy and the probed model capability, and transparently retries once
// without thinking if LlamaWow rejects the request for asking.
//
// Blocking. Call from a worker thread, never from the world thread.
LlamaWowApiResult QueryLlamaWow(const std::string& prompt, LlamaWowRequestKind kind);

// Legacy shim: returns the text, or empty on any failure.
std::string QueryLlamaWowAPI(const std::string& prompt);

bool IsValidAPIResponse(const std::string& response);

#endif // MOD_LLAMA_WOW_API_H
