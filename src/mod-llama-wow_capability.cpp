#include "mod-llama-wow_capability.h"
#include "mod-llama-wow_config.h"
#include "mod-llama-wow_httpclient.h"
#include "mod-llama-wow-utilities.h"

#include "Log.h"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <mutex>
#include <nlohmann/json.hpp>
#include <string>
#include <thread>

namespace
{
    std::mutex g_mutex;

    LlamaWowThinkSupport g_support = LlamaWowThinkSupport::Unknown;
    std::string        g_probedUrl;
    std::string        g_probedModel;
    std::string        g_probeDetail = "not probed yet";
    bool               g_probeRunning = false;

    std::atomic<bool>     g_latencyDisabled{ false };
    std::atomic<bool>     g_rejectionLogged{ false };
    std::atomic<bool>     g_reasonsUnconditionally{ false };
    std::atomic<bool>     g_effortLevelsRejected{ false };

    // Guarded by g_latencyMutex rather than done with atomics: a running mean
    // needs sum and count to move together or the average is nonsense.
    std::mutex g_latencyMutex;
    uint64_t   g_thinkSamples   = 0;
    uint64_t   g_thinkLatencySum = 0;

    uint64_t ThinkLatencyMean()
    {
        std::lock_guard<std::mutex> lock(g_latencyMutex);
        return g_thinkSamples ? (g_thinkLatencySum / g_thinkSamples) : 0;
    }

    uint64_t ThinkLatencySamples()
    {
        std::lock_guard<std::mutex> lock(g_latencyMutex);
        return g_thinkSamples;
    }

    std::string ToLower(std::string v)
    {
        for (char& c : v)
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return v;
    }

    void SetSupport(LlamaWowThinkSupport support, const std::string& detail)
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        g_support     = support;
        g_probeDetail = detail;
    }

    // Step 1: ask LlamaWow what the model can do. LlamaWow 0.6+ returns a
    // "capabilities" array on /api/show.
    // Returns true when the answer was conclusive.
    bool ProbeViaShow(LlamaWowHttpClient& http, const std::string& base,
                      const std::string& model, bool& outSupported,
                      std::string& outDetail)
    {
        nlohmann::json body = { { "model", model } };

        LlamaWowHttpResult r = http.PostEx(base + "/api/show", body.dump(),
                                         static_cast<int>(g_CapabilityProbeTimeoutSeconds));

        if (!r.ok())
        {
            outDetail = r.error.empty()
                            ? ("HTTP " + std::to_string(r.status) + " from /api/show")
                            : r.error;
            return false;
        }

        try
        {
            nlohmann::json parsed = nlohmann::json::parse(r.body);
            if (!parsed.contains("capabilities") || !parsed["capabilities"].is_array())
                return false;   // older LlamaWow: fall through to the live probe

            for (const auto& cap : parsed["capabilities"])
            {
                if (cap.is_string() && ToLower(cap.get<std::string>()) == "thinking")
                {
                    outSupported = true;
                    outDetail    = "/api/show reports the thinking capability";
                    return true;
                }
            }

            outSupported = false;
            outDetail    = "/api/show does not list a thinking capability";
            return true;
        }
        catch (const std::exception& e)
        {
            outDetail = std::string("could not parse /api/show: ") + e.what();
            return false;
        }
    }

    // Step 2 (older LlamaWow, or an inconclusive /api/show): actually ask for a
    // one-token thinking generation and see whether it is refused.
    bool ProbeViaGenerate(LlamaWowHttpClient& http, const std::string& base,
                          const std::string& model, bool& outSupported,
                          std::string& outDetail)
    {
        nlohmann::json body = {
            { "model",  model },
            { "prompt", "hi" },
            { "stream", false },
            { "think",  true },
            { "options", { { "num_predict", 1 } } },
        };

        LlamaWowHttpResult r = http.PostEx(base + "/api/generate", body.dump(),
                                         static_cast<int>(g_CapabilityProbeTimeoutSeconds));

        if (r.ok())
        {
            outSupported = true;
            outDetail    = "live probe accepted a thinking request";
            return true;
        }

        if (LlamaWowCapability_IsThinkRejection(r.status, r.body))
        {
            outSupported = false;
            outDetail    = "live probe rejected: model does not support thinking";
            return true;
        }

        outDetail = r.error.empty()
                        ? ("live probe failed with HTTP " + std::to_string(r.status))
                        : ("live probe failed: " + r.error);
        return false;
    }

    void RunProbe(std::string base, std::string model)
    {
        LlamaWowHttpClient http;

        bool supported = false;
        std::string detail;

        bool conclusive = ProbeViaShow(http, base, model, supported, detail);
        if (!conclusive)
        {
            std::string showDetail = detail;
            conclusive = ProbeViaGenerate(http, base, model, supported, detail);
            if (!conclusive && !showDetail.empty())
                detail = showDetail + "; " + detail;
        }

        if (!conclusive)
        {
            SetSupport(LlamaWowThinkSupport::ProbeFailed, detail);
            LOG_WARN("module.mod_llama_wow",
                     "[LlamaWow Chat] Think-mode probe inconclusive for model '{}' ({}). "
                     "Proceeding without think mode.", model, detail);
        }
        else
        {
            SetSupport(supported ? LlamaWowThinkSupport::Supported
                                 : LlamaWowThinkSupport::Unsupported,
                       detail);
            LOG_INFO("module.mod_llama_wow",
                     "[LlamaWow Chat] Think mode {} for model '{}' ({}).",
                     supported ? "AVAILABLE" : "not available", model, detail);
        }

        std::lock_guard<std::mutex> lock(g_mutex);
        g_probeRunning = false;
    }
}

// --------------------------------------------------------------------------

void LlamaWowCapability_Init(bool force)
{
    // [MRafStudio fork] В OpenAI-совместимом режиме нативного /api/show нет
    // (llama.cpp/vLLM отвечают 404), поэтому пробу не запускаем вовсе:
    // думание считается недоступным, а в лог не сыплются ложные ошибки.
    if (g_LlamaWowApiMode == "openai" || g_LlamaWowApiMode == "openai-compatible" || g_LlamaWowApiMode == "v1")
    {
        SetSupport(LlamaWowThinkSupport::Unsupported,
                   "openai-compatible mode: /api/show probe skipped");
        return;
    }

    const std::string base  = LlamaWowDeriveBaseUrl(g_LlamaWowUrl);
    const std::string model = g_LlamaWowModel;

    {
        std::lock_guard<std::mutex> lock(g_mutex);

        if (g_probeRunning)
            return;

        if (!force && base == g_probedUrl && model == g_probedModel &&
            g_support != LlamaWowThinkSupport::Unknown)
        {
            return;   // nothing changed
        }

        g_probedUrl   = base;
        g_probedModel = model;
        g_support     = LlamaWowThinkSupport::Unknown;
        g_probeDetail = "probe in progress";
        g_probeRunning = true;
    }

    g_latencyDisabled.store(false);
    g_rejectionLogged.store(false);
    g_reasonsUnconditionally.store(false);
    g_effortLevelsRejected.store(false);
    {
        std::lock_guard<std::mutex> lock(g_latencyMutex);
        g_thinkSamples    = 0;
        g_thinkLatencySum = 0;
    }

    // Off the world thread: a missing LlamaWow must not stall startup.
    std::thread(RunProbe, base, model).detach();
}

LlamaWowThinkSupport LlamaWowCapability_GetSupport()
{
    std::lock_guard<std::mutex> lock(g_mutex);
    return g_support;
}

bool LlamaWowCapability_SupportsThinking()
{
    return LlamaWowCapability_GetSupport() == LlamaWowThinkSupport::Supported;
}

bool LlamaWowCapability_IsThinkRejection(int status, const std::string& body)
{
    if (status != 400 && status != 422 && status != 500)
        return false;

    const std::string lower = ToLower(body);
    return lower.find("does not support thinking") != std::string::npos ||
           lower.find("does not support think")    != std::string::npos ||
           lower.find("thinking is not supported") != std::string::npos ||
           lower.find("unsupported parameter: think") != std::string::npos;
}

void LlamaWowCapability_NoteThinkRejected()
{
    SetSupport(LlamaWowThinkSupport::Unsupported,
               "runtime rejection: model reported it cannot think");

    if (!g_rejectionLogged.exchange(true))
    {
        LOG_INFO("module.mod_llama_wow",
                 "[LlamaWow Chat] Model '{}' rejected a thinking request; think mode is now "
                 "off for this model. Requests are retried without it automatically.",
                 g_LlamaWowModel);
    }
}

void LlamaWowCapability_NoteUnconditionalReasoning()
{
    if (g_reasonsUnconditionally.exchange(true))
        return;

    LOG_INFO("module.mod_llama_wow",
             "[LlamaWow Chat] Model '{}' produced reasoning despite think being off, and spent "
             "the whole NumPredict budget on it. Reasoning headroom is now added to every "
             "request for this model (mod_llama_wow.ReasoningTokenReserve = {}).",
             g_LlamaWowModel, g_ReasoningTokenReserve);
}

bool LlamaWowCapability_ReasonsUnconditionally()
{
    return g_reasonsUnconditionally.load();
}

void LlamaWowCapability_NoteEffortLevelRejected()
{
    if (g_effortLevelsRejected.exchange(true))
        return;

    LOG_INFO("module.mod_llama_wow",
             "[LlamaWow Chat] Model '{}' does not take a string reasoning level; falling back to "
             "the boolean think field for the rest of this session.", g_LlamaWowModel);
}

bool LlamaWowCapability_EffortLevelsRejected()
{
    return g_effortLevelsRejected.load();
}

LlamaWowThinkRequest LlamaWowCapability_ResolveThink(LlamaWowRequestKind kind)
{
    LlamaWowThinkRequest req;
    req.wanted  = LlamaWowCapability_ShouldThink(kind);
    req.enabled = req.wanted;

    // Reasoning was not wanted here, but this model has already shown it
    // reasons regardless. Asking for the lowest effort level is the closest
    // thing to off that such a model offers; a plain false is ignored.
    if (!req.wanted &&
        g_reasonsUnconditionally.load() &&
        !g_effortLevelsRejected.load())
    {
        req.level = "low";
    }

    return req;
}

void LlamaWowCapability_NoteLatency(uint64_t milliseconds, bool thinkUsed)
{
    if (!thinkUsed || g_ThinkMaxLatencyMs == 0 || g_latencyDisabled.load())
        return;

    uint64_t n = 0;
    uint64_t mean = 0;
    {
        std::lock_guard<std::mutex> lock(g_latencyMutex);
        ++g_thinkSamples;
        g_thinkLatencySum += milliseconds;
        n    = g_thinkSamples;
        mean = g_thinkLatencySum / g_thinkSamples;
    }

    // Require a few samples so one cold start does not trip the guard.
    if (n >= 3 && mean > g_ThinkMaxLatencyMs)
    {
        if (!g_latencyDisabled.exchange(true))
        {
            LOG_INFO("module.mod_llama_wow",
                     "[LlamaWow Chat] Think mode averaged {}ms over {} requests, above the "
                     "{}ms budget. Disabling think for the rest of this session so replies "
                     "stay responsive.", mean, n, g_ThinkMaxLatencyMs);
        }
    }
}

bool LlamaWowCapability_ThinkDisabledByLatency()
{
    return g_latencyDisabled.load();
}

bool LlamaWowCapability_ShouldThink(LlamaWowRequestKind kind)
{
    const LlamaWowThinkPolicy policy = static_cast<LlamaWowThinkPolicy>(g_ThinkModePolicy);

    if (policy == LlamaWowThinkPolicy::Off)
        return false;

    // Never ask a model to think when we know it cannot; that is the request
    // that used to come back as an unexplained empty reply.
    if (!LlamaWowCapability_SupportsThinking())
        return false;

    if (g_latencyDisabled.load())
        return false;

    if (policy == LlamaWowThinkPolicy::On)
        return true;

    // Auto: spend reasoning only where it changes the answer.
    switch (kind)
    {
        case LlamaWowRequestKind::Sentiment:
            return true;    // a judgement, and never shown to players
        case LlamaWowRequestKind::RoleplayReply:
            return g_RoleplayStrictness >= 2;
        case LlamaWowRequestKind::ChatReply:
        case LlamaWowRequestKind::RandomChatter:
        case LlamaWowRequestKind::EventChatter:
        default:
            return false;   // short, latency-sensitive, nothing to reason about
    }
}

std::string LlamaWowCapability_StatusText()
{
    std::string supportText;
    std::string detail;

    {
        std::lock_guard<std::mutex> lock(g_mutex);
        switch (g_support)
        {
            case LlamaWowThinkSupport::Supported:   supportText = "supported";   break;
            case LlamaWowThinkSupport::Unsupported: supportText = "unsupported"; break;
            case LlamaWowThinkSupport::ProbeFailed: supportText = "probe failed"; break;
            default:                              supportText = "probing...";  break;
        }
        detail = g_probeDetail;
    }

    const char* policyText = "auto";
    switch (static_cast<LlamaWowThinkPolicy>(g_ThinkModePolicy))
    {
        case LlamaWowThinkPolicy::On:  policyText = "on";  break;
        case LlamaWowThinkPolicy::Off: policyText = "off"; break;
        default:                     policyText = "auto"; break;
    }

    std::string out = SafeFormat("policy={} support={} ({})",
                                 policyText, supportText, detail);

    if (g_reasonsUnconditionally.load())
    {
        out += SafeFormat(" [model reasons unconditionally; +{} token reserve, off sent as {}]",
                          g_ReasoningTokenReserve,
                          g_effortLevelsRejected.load() ? "think:false" : "think:\"low\"");
    }

    if (g_latencyDisabled.load())
    {
        out += SafeFormat(" [disabled by latency guard: mean {}ms > {}ms]",
                          ThinkLatencyMean(), g_ThinkMaxLatencyMs);
    }
    else if (ThinkLatencySamples() > 0)
    {
        out += SafeFormat(" [mean think latency {}ms over {} requests]",
                          ThinkLatencyMean(), ThinkLatencySamples());
    }

    return out;
}

LlamaWowThinkPolicy LlamaWowCapability_ParsePolicy(const std::string& text, bool legacyBool)
{
    const std::string v = ToLower(text);

    if (v == "auto" || v.empty())
    {
        // The deprecated bool still forces think on when it was explicitly set.
        return legacyBool ? LlamaWowThinkPolicy::On : LlamaWowThinkPolicy::Auto;
    }
    if (v == "on" || v == "1" || v == "true" || v == "yes" || v == "always")
        return LlamaWowThinkPolicy::On;
    if (v == "off" || v == "0" || v == "false" || v == "no" || v == "never")
        return LlamaWowThinkPolicy::Off;

    LOG_WARN("module.mod_llama_wow",
             "[LlamaWow Chat] Unrecognised ThinkMode value '{}'; using auto.", text);
    return LlamaWowThinkPolicy::Auto;
}
