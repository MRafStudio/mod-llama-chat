#ifndef MOD_LLAMA_WOW_CAPABILITY_H
#define MOD_LLAMA_WOW_CAPABILITY_H

#include <string>
#include <cstdint>

// --------------------------------------------------------------------------
// Think-mode capability detection and policy.
//
// Two separate questions the old code conflated into one bool:
//
//   CAN this model think?   -- answered by probing LlamaWow, cached per
//                              (url, model), and self-healed at runtime when a
//                              request comes back rejected.
//   SHOULD it think here?   -- answered per request kind. Reasoning costs
//                              seconds and tokens; a fifteen-word tavern line
//                              does not need it, a sentiment judgement does.
//
// Getting the first one wrong used to silence every bot on the server with
// nothing in the log but a generic empty-response error.
// --------------------------------------------------------------------------

enum class LlamaWowThinkPolicy : uint8_t
{
    Auto = 0,   // capability-gated and kind-gated (default)
    On   = 1,   // always think when the model supports it
    Off  = 2,   // never think
};

enum class LlamaWowRequestKind : uint8_t
{
    ChatReply = 0,
    RandomChatter,
    EventChatter,
    Sentiment,
    RoleplayReply,
};

enum class LlamaWowThinkSupport : uint8_t
{
    Unknown = 0,      // probe has not completed yet
    Supported,
    Unsupported,
    ProbeFailed,      // could not reach LlamaWow; treated as unsupported
};

// Kick off (or re-run) the capability probe. Runs on a background thread so a
// missing or slow LlamaWow cannot stall worldserver startup. Safe to call again
// after a config reload; re-probes when url or model changed, or when forced.
void LlamaWowCapability_Init(bool force = false);

LlamaWowThinkSupport LlamaWowCapability_GetSupport();
bool LlamaWowCapability_SupportsThinking();

// The policy decision for one request: does this kind of request want the
// model to reason at all?
bool LlamaWowCapability_ShouldThink(LlamaWowRequestKind kind);

// What actually goes into the request's "think" field.
//
// LlamaWow takes either a bool or, on models with reasoning-effort levels
// (gpt-oss and other harmony builds), one of "low"/"medium"/"high". A model
// that ignores `think: false` cannot be silenced -- but it can be turned down.
// So for those models "off" resolves to the lowest level they accept rather
// than to a false they have already demonstrated they ignore.
struct LlamaWowThinkRequest
{
    bool        wanted  = false;   // did the policy want reasoning here?
    bool        enabled = false;   // the bool to send when `level` is empty
    std::string level;             // "low"/"medium"/"high", or empty for a bool
};

// Resolves policy plus everything learned about this model at runtime into the
// field to send. This is the on-the-fly configuration: nothing else decides.
LlamaWowThinkRequest LlamaWowCapability_ResolveThink(LlamaWowRequestKind kind);

// Self-heal: LlamaWow refused a string reasoning level, so this model takes the
// boolean form only. Falls back permanently for this model.
void LlamaWowCapability_NoteEffortLevelRejected();
bool LlamaWowCapability_EffortLevelsRejected();

// True when an HTTP failure is LlamaWow telling us the model cannot think.
bool LlamaWowCapability_IsThinkRejection(int status, const std::string& body);

// Self-heal: called when a live request was rejected for asking to think.
// Flips the cached support flag off and logs once.
void LlamaWowCapability_NoteThinkRejected();

// `think: false` is a request, not a guarantee. gpt-oss and other
// harmony-format builds reason unconditionally, and LlamaWow counts those tokens
// against num_predict -- so a small cap is spent entirely on reasoning and the
// answer channel never opens. The result is HTTP 200 with an empty "response"
// and a "thinking" field truncated mid-word.
//
// Noted the first time we see that shape, so every later request for this
// model is budgeted with reasoning headroom from the start.
void LlamaWowCapability_NoteUnconditionalReasoning();
bool LlamaWowCapability_ReasonsUnconditionally();

// Latency guard: feeds the rolling average that auto mode uses to back off
// think mode when the model turns out to be too slow for chat.
void LlamaWowCapability_NoteLatency(uint64_t milliseconds, bool thinkUsed);

// True once the latency guard has disabled think for the rest of the session.
bool LlamaWowCapability_ThinkDisabledByLatency();

// Human-readable summary for the .llamawow status command.
std::string LlamaWowCapability_StatusText();

// Parse "auto" / "on" / "off" (and the legacy bool) into a policy.
LlamaWowThinkPolicy LlamaWowCapability_ParsePolicy(const std::string& text, bool legacyBool);

#endif // MOD_LLAMA_WOW_CAPABILITY_H
