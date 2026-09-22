#include "mod-llama-wow_config.h"
#include "Common.h"
#include "SharedDefines.h"
#include "mod-llama-wow_sentiment.h"
#include "mod-llama-wow_expression.h"
#include "mod-llama-wow_handler.h"
#include "mod-llama-wow_rag.h"
#include "Config.h"
#include "Log.h"
#include "mod-llama-wow_api.h"
#include "mod-llama-wow_capability.h"
#include "mod-llama-wow_dispatch.h"
#include "mod-llama-wow_memory.h"
#include "mod-llama-wow_roleplay.h"
#include "mod-llama-wow_topics.h"
#include "mod-llama-wow-utilities.h"
#include <fmt/core.h>
#include <sstream>
#include <fstream>


// --------------------------------------------
// Distance/Range Configuration
// --------------------------------------------
float      g_SayDistance       = 30.0f;
float      g_YellDistance      = 100.0f;
float      g_RandomChatterRealPlayerDistance = 40.0f;
float      g_EventChatterRealPlayerDistance = 40.0f;

// --------------------------------------------
// Bot/Player Chatter Probability & Limits
// --------------------------------------------
// Per-channel-type reply chances
uint32_t   g_PlayerReplyChance_Say     = 90;
uint32_t   g_BotReplyChance_Say        = 10;
uint32_t   g_PlayerReplyChance_Channel = 50;
uint32_t   g_BotReplyChance_Channel    = 5;
uint32_t   g_PlayerReplyChance_Party   = 90;
uint32_t   g_BotReplyChance_Party      = 10;
uint32_t   g_PlayerReplyChance_Guild   = 70;
uint32_t   g_BotReplyChance_Guild      = 5;

uint32_t   g_MaxBotsToPick     = 2;
uint32_t   g_RandomChatterBotCommentChance   = 5;
uint32_t   g_RandomChatterMaxBotsPerPlayer   = 2;
uint32_t   g_EventChatterBotCommentChance    = 15;
uint32_t   g_EventChatterBotSelfCommentChance = 5;
uint32_t   g_EventChatterMaxBotsPerPlayer    = 2;

// --------------------------------------------
// LlamaWow LLM API Configuration
// --------------------------------------------
std::string g_LlamaWowUrl        = "http://localhost:11434/api/generate";
std::string g_LlamaWowModel      = "llama3.2:1b";
uint32_t    g_LlamaWowNumPredict = 40;
float       g_LlamaWowTemperature = 0.8f;
float       g_LlamaWowTopP = 0.95f;
float       g_LlamaWowRepeatPenalty = 1.1f;
uint32_t    g_LlamaWowNumCtx = 0;
uint32_t    g_LlamaWowNumThreads = 0;
std::string g_LlamaWowStop = "";
std::string g_LlamaWowSystemPrompt = "";

// [MRafStudio fork] Протокол обращения к LLM и токен доступа.
// "ollama" -- как раньше (нативный /api/generate); "openai" -- /v1/chat/completions.
std::string g_LlamaWowApiMode = "ollama";
std::string g_LlamaWowApiKey = "";
bool        g_LlamaWowOpenAiDisableThinking = true;
std::string g_LlamaWowSeed = "";
int32_t     g_LlamaWowTopK             = -1;
float       g_LlamaWowMinP             = -1.0f;
float       g_LlamaWowPresencePenalty  = -1000.0f;
float       g_LlamaWowFrequencyPenalty = -1000.0f;

// --------------------------------------------
// Concurrency/Queueing
// --------------------------------------------
uint32_t    g_MaxConcurrentQueries = 0;

// --------------------------------------------
// Feature Toggles & Core Settings
// --------------------------------------------
bool        g_Enable                          = true;
bool        g_DisableRepliesInCombat          = true;
bool        g_EnableRandomChatter             = true;
bool        g_EnableEventChatter              = true;
bool        g_EnableRPPersonalities           = false;
bool        g_EnableWhisperReplies            = false;
bool        g_DebugEnabled                    = false;
bool        g_DebugShowFullPrompt             = false;

// --------------------------------------------
// Think Mode
// --------------------------------------------
bool     g_ThinkModeEnableForModule      = false;
uint8_t  g_ThinkModePolicy               = 0;      // LlamaWowThinkPolicy::Auto
uint32_t g_ThinkMaxLatencyMs             = 12000;
uint32_t g_ReasoningTokenReserve         = 512;
uint32_t g_CapabilityProbeTimeoutSeconds = 10;

// --------------------------------------------
// HTTP / dispatcher
// --------------------------------------------
uint32_t g_HttpTimeoutSeconds     = 120;
uint32_t g_DispatchWorkerThreads  = 4;
uint32_t g_MaxQueueDepth          = 64;

// --------------------------------------------
// Response post-processing
// --------------------------------------------
uint32_t g_MaxReplyLength                = 240;
bool     g_ResponseStripMarkdown         = true;
bool     g_ResponseStripDecorativeUnicode = true;

// --------------------------------------------
// Conversation governor
// --------------------------------------------
uint8_t  g_MaxChainDepth                 = 3;
uint32_t g_ChainChanceDecayPct           = 50;
bool     g_RequireRecentHuman            = true;
uint32_t g_HumanWindowSeconds            = 120;
uint32_t g_BotCooldownSeconds            = 45;
uint32_t g_ScopeCooldownSeconds          = 15;
uint32_t g_DirectAddressGroupSize        = 5;
uint32_t g_ConversationWindowSeconds     = 180;
uint32_t g_DirectAddressReplyChance      = 95;
uint32_t g_ScopeMessagesPerMinute        = 8;
uint32_t g_GlobalMessagesPerMinute       = 40;
uint32_t g_BotHistorySize                = 12;
uint32_t g_ScopeHistorySize              = 30;
float    g_RepetitionSimilarityThreshold = 0.72f;
uint32_t g_RepetitionWindowSeconds       = 1800;
uint32_t g_OpenerHistorySize             = 8;

// --------------------------------------------
// Topic engine
// --------------------------------------------
uint32_t g_TopicWeightPeople        = 30;
uint32_t g_TopicWeightWorld         = 30;
uint32_t g_TopicWeightActivity      = 25;
uint32_t g_TopicWeightSelf          = 15;
uint32_t g_TopicWeightGuild         = 20;
uint32_t g_TopicMemoryCount         = 4;
uint32_t g_TopicEventMemorySize     = 6;
uint32_t g_TopicEventMemorySeconds  = 300;
float    g_TopicPlayerRadius        = 40.0f;
uint32_t g_RandomChatterQuestionChance = 35;

std::vector<std::string> g_EnvCommentNearbyPlayer;
std::vector<std::string> g_EnvCommentGroupMember;
std::vector<std::string> g_EnvCommentGuildMemberOnline;
std::vector<std::string> g_EnvCommentRecentEvent;
std::vector<std::string> g_EnvCommentNamedNpc;
std::vector<std::string> g_EnvCommentZoneLandmark;
std::vector<std::string> g_EnvCommentTimeOfDay;
std::vector<std::string> g_EnvCommentCorpse;
std::vector<std::string> g_EnvCommentDanger;
std::vector<std::string> g_EnvCommentQuestObjective;
std::vector<std::string> g_EnvCommentGroupNeed;

// --------------------------------------------
// Game-state snapshot limits
// --------------------------------------------
bool     g_SnapshotIncludeSpells = false;
uint32_t g_SnapshotMaxSpells     = 6;
uint32_t g_SnapshotMaxCreatures  = 8;
uint32_t g_SnapshotMaxObjects    = 8;
uint32_t g_SnapshotMaxPlayers    = 8;

// --------------------------------------------
// Roleplay mode
// --------------------------------------------
bool        g_RoleplayEnable               = false;
uint8_t     g_RoleplayStrictness           = 1;
bool        g_RoleplayUseRaceVoice         = true;
bool        g_RoleplayUseClassVoice        = true;
bool        g_RoleplayFactionAttitude      = true;
bool        g_RoleplayBlockMetaTerms       = true;
std::string g_RoleplayMetaTermList;
bool        g_RoleplayCrossFactionGibberish = true;
std::vector<std::string> g_RoleplayPromptVariations;
std::vector<std::string> g_RoleplayQuestionVariations;

// --------------------------------------------
// Embodiment
// --------------------------------------------
bool        g_EnableBotFacing              = true;
bool        g_EnableBotEmotes              = true;
uint32_t    g_StateEmoteDurationMs         = 15000;
int         g_GestureChance                = 40;
uint32_t    g_EmoteFallbackId              = 0;
uint32_t    g_BotExpressionDelayMs         = 400;
float       g_BotFacingMaxDistance         = 40.0f;
bool        g_EnableEmoteReactions         = true;
uint32_t    g_EmoteReplyChance             = 60;
uint32_t    g_EmoteReplyMirrorWeight       = 55;
uint32_t    g_EmoteReplyCounterWeight      = 30;
uint32_t    g_EmoteReplySpeakWeight        = 15;
uint32_t    g_EmoteReactionCooldownSeconds = 20;
std::string g_EmoteReactionPromptTemplate;

// --------------------------------------------
// Long-term memory and relationships
// --------------------------------------------
bool        g_MemoryEnable             = true;
uint32_t    g_MemoryHistoryTokenLimit  = 1500;
uint32_t    g_MemoryPromptTokenBudget  = 400;
uint32_t    g_MemoryMaxPerBot          = 40;
uint32_t    g_MemorySaveInterval       = 10;
std::string g_MemoryCondensePrompt;
std::string g_MemoryPromptTemplate;

bool        g_RelationshipEnable            = true;
uint32_t    g_RelationshipMentionThreshold  = 8;
uint32_t    g_RelationshipMaxPerPrompt      = 3;
uint32_t    g_RelationshipMaxLength         = 220;
std::string g_RelationshipUpdatePrompt;
std::string g_RelationshipPromptTemplate;

time_t      g_LastMemorySaveTime = time(nullptr);

// --------------------------------------------
// Random Chatter Timing
// --------------------------------------------
uint32_t    g_MinRandomInterval               = 45;
uint32_t    g_MaxRandomInterval               = 180;

// --------------------------------------------
// Conversation History Settings
// --------------------------------------------
uint32_t    g_MaxConversationHistory          = 5;
uint32_t    g_ConversationHistorySaveInterval = 10;

// --------------------------------------------
// Prompt Templates
// --------------------------------------------
std::string g_RandomChatterPromptTemplate;
std::vector<std::string> g_RandomChatterPromptVariations;
std::vector<std::string> g_RandomChatterQuestionVariations;
std::string g_EventChatterPromptTemplate;
std::string g_ChatPromptTemplate;
std::string g_ChatExtraInfoTemplate;

// --------------------------------------------
// Personality and Prompt Data
// --------------------------------------------
std::unordered_map<uint64_t, std::string> g_BotPersonalityList;
std::unordered_map<std::string, std::string> g_PersonalityPrompts;
std::unordered_map<std::string, std::unordered_map<uint8, std::string>> g_PersonalityPromptsLocale;
std::vector<std::string> g_PersonalityKeys;
std::vector<std::string> g_PersonalityKeysRandomOnly;
std::string g_DefaultPersonalityPrompt;

// --------------------------------------------
// Chat History Templates and Toggles
// --------------------------------------------
bool        g_EnableChatHistory = true;
std::string g_ChatHistoryHeaderTemplate;
std::string g_ChatHistoryLineTemplate;
std::string g_ChatHistoryFooterTemplate;

// --------------------------------------------
// Chatbot Snapshot Template
// --------------------------------------------
bool        g_EnableChatBotSnapshotTemplate  = false;
std::string g_ChatBotSnapshotTemplate;

// --------------------------------------------
// Conversation History Store and Mutex
// --------------------------------------------
std::unordered_map<uint64_t, std::unordered_map<uint64_t, std::deque<BotConversationEntry>>> g_BotConversationHistory;
std::mutex g_ConversationHistoryMutex;
// Seeded at startup, not left at 0. difftime(now, 0) is ~56 years, so the old
// value made the very first world tick fire a full save of a store that had
// only just been loaded from the same table.
time_t g_LastHistorySaveTime = time(nullptr);

// --------------------------------------------
// Bot-Player Sentiment Tracking System
// --------------------------------------------
bool        g_EnableSentimentTracking = true;
float       g_SentimentDefaultValue = 0.5f;              // Default sentiment value (0.5 = neutral)
float       g_SentimentAdjustmentStrength = 0.1f;        // How much to adjust sentiment per message
uint32_t    g_SentimentSaveInterval = 10;                // How often to save sentiment to DB (minutes)
std::string g_SentimentAnalysisPrompt = "Analyze the sentiment of this message: \"{message}\". Respond only with: POSITIVE, NEGATIVE, or NEUTRAL.";
std::string g_SentimentPromptTemplate = "Your relationship sentiment with {player_name} is {sentiment_value} (0.0=hostile, 0.5=neutral, 1.0=friendly). Use this to guide your tone and response.";

// In-memory sentiment storage and mutex
std::unordered_map<uint64_t, std::unordered_map<uint64_t, float>> g_BotPlayerSentiments;
std::set<std::pair<uint64_t, uint64_t>> g_DirtySentiments;
std::mutex g_SentimentMutex;
time_t g_LastSentimentSaveTime = time(nullptr);

// --------------------------------------------
// RAG (Retrieval-Augmented Generation) System
// --------------------------------------------
bool        g_EnableRAG = false;
std::string g_RAGDataPath = "rag/";
uint32_t    g_RAGMaxRetrievedItems = 3;
float       g_RAGSimilarityThreshold = 0.3f;
std::string g_RAGPromptTemplate;

class LlamaWowRAGSystem;
LlamaWowRAGSystem* g_RAGSystem = nullptr;

// --------------------------------------------
// Blacklist: Prefixes for Commands (not chat)
// --------------------------------------------
std::vector<std::string> g_BlacklistCommands = {
    ".playerbots",
    "playerbot",
};

// --------------------------------------------
// Environment/Contextual Random Chatter Templates
// --------------------------------------------
std::vector<std::string> g_EnvCommentCreature;
std::vector<std::string> g_EnvCommentGameObject;
std::vector<std::string> g_EnvCommentEquippedItem;
std::vector<std::string> g_EnvCommentBagItem;
std::vector<std::string> g_EnvCommentBagItemSell;
std::vector<std::string> g_EnvCommentSpell;
std::vector<std::string> g_EnvCommentQuestArea;
std::vector<std::string> g_EnvCommentVendor;
std::vector<std::string> g_EnvCommentQuestgiver;
std::vector<std::string> g_EnvCommentBagSlots;
std::vector<std::string> g_EnvCommentDungeon;
std::vector<std::string> g_EnvCommentUnfinishedQuest;

// --------------------------------------------
// Guild-Specific Random Chatter Templates
// --------------------------------------------
std::vector<std::string> g_GuildEnvCommentGuildMember;
std::vector<std::string> g_GuildEnvCommentGuildRank;
std::vector<std::string> g_GuildEnvCommentGuildBank;
std::vector<std::string> g_GuildEnvCommentGuildMOTD;
std::vector<std::string> g_GuildEnvCommentGuildInfo;
std::vector<std::string> g_GuildEnvCommentGuildOnlineMembers;
std::vector<std::string> g_GuildEnvCommentGuildRaid;
std::vector<std::string> g_GuildEnvCommentGuildEndgame;
std::vector<std::string> g_GuildEnvCommentGuildStrategy;
std::vector<std::string> g_GuildEnvCommentGuildGroup;
std::vector<std::string> g_GuildEnvCommentGuildPvP;
std::vector<std::string> g_GuildEnvCommentGuildCommunity;

// --------------------------------------------
// Guild-Specific Random Chatter Configuration
// --------------------------------------------
bool        g_EnableGuildEventChatter             = true;
bool        g_EnableGuildRandomAmbientChatter      = true;
uint32_t    g_GuildRandomChatterChance             = 10;
uint32_t    g_GuildChatterBotCommentChance          = 25;
uint32_t    g_GuildChatterMaxBotsPerEvent           = 2;

// --------------------------------------------
// Guild-Specific Event Chatter Templates
// --------------------------------------------
std::string g_GuildEventTypeLevelUp = "";
std::string g_GuildEventTypeDungeonComplete = "";
std::string g_GuildEventTypeEpicGear = "";
std::string g_GuildEventTypeRareGear = "";
std::string g_GuildEventTypeGuildJoin = "";
std::string g_GuildEventTypeGuildLeave = "";
std::string g_GuildEventTypeGuildPromotion = "";
std::string g_GuildEventTypeGuildDemotion = "";
std::string g_GuildEventTypeGuildLogin = "";
std::string g_GuildEventTypeGuildAchievement = "";

// --------------------------------------------
// Event Chatter Templates
// --------------------------------------------
std::string g_EventTypeDefeated;           // "defeated"
std::string g_EventTypeDefeatedPlayer;     // "defeated player"
std::string g_EventTypePetDefeated;        // "pet defeated"
std::string g_EventTypeGotItem;            // "got item"
std::string g_EventTypeDied;               // "died"
std::string g_EventTypeCompletedQuest;     // "completed quest"
std::string g_EventTypeLearnedSpell;       // "learned spell"
std::string g_EventTypeRequestedDuel;      // "requested to duel"
std::string g_EventTypeStartedDueling;     // "started dueling"
std::string g_EventTypeWonDuel;            // "won duel against"
std::string g_EventTypeLeveledUp;          // "leveled up"
std::string g_EventTypeAchievement;        // "earned achievement"
std::string g_EventTypeUsedObject;         // "used object"

// Chance variables for normal events
int g_EventTypeDefeated_Chance = 0;
int g_EventTypeDefeatedPlayer_Chance = 0;
int g_EventTypePetDefeated_Chance = 0;
int g_EventTypeGotItem_Chance = 0;
int g_EventTypeDied_Chance = 0;
int g_EventTypeCompletedQuest_Chance = 0;
int g_EventTypeLearnedSpell_Chance = 0;
int g_EventTypeRequestedDuel_Chance = 0;
int g_EventTypeStartedDueling_Chance = 0;
int g_EventTypeWonDuel_Chance = 0;
int g_EventTypeLeveledUp_Chance = 0;
int g_EventTypeAchievement_Chance = 0;
int g_EventTypeUsedObject_Chance = 0;

// Chance variables for guild events
int g_GuildEventTypeEpicGear_Chance = 0;
int g_GuildEventTypeRareGear_Chance = 0;
int g_GuildEventTypeGuildJoin_Chance = 0;
int g_GuildEventTypeGuildLogin_Chance = 0;
int g_GuildEventTypeGuildLeave_Chance = 0;
int g_GuildEventTypeGuildPromotion_Chance = 0;
int g_GuildEventTypeGuildDemotion_Chance = 0;
int g_GuildEventTypeGuildAchievement_Chance = 0;
int g_GuildEventTypeLevelUp_Chance = 0;
int g_GuildEventTypeDungeonComplete_Chance = 0;

// Event Cooldown
uint32_t g_EventCooldownTime = 10;

// --------------------------------------------
// Channel Disable Settings
// --------------------------------------------
bool g_DisableForCustomChannels = false;
bool g_DisableForSayYell = false;
bool g_DisableForGuild = false;
bool g_DisableForParty = false;

bool g_ChatterUseGeneralChannel           = true;
bool g_ChatterUseTradeChannel             = true;
bool g_ChatterUseLfgChannel               = false;
bool g_ChatterUseGuildRecruitmentChannel  = false;

// --------------------------------------------
// Typing Simulation Settings
// --------------------------------------------
bool g_EnableTypingSimulation = false;
uint32_t g_TypingSimulationBaseDelay = 1000;     // 1000ms base delay
uint32_t g_TypingSimulationDelayPerChar = 250;
uint32_t g_TypingSimulationMaxDelay     = 8000;   // 250ms per character (4 chars/sec)

// Load Bot Personalities from Database
void LoadBotPersonalityList()
{    
    // Let's make sure our user has sourced the required sql file to add the new table
    QueryResult tableExists = CharacterDatabase.Query("SELECT * FROM information_schema.tables WHERE table_schema = 'acore_characters' AND table_name = 'mod_llama_wow_personality' LIMIT 1");
    if (!tableExists)
    {
        LOG_ERROR("module.mod_llama_wow", "[LlamaWow Chat] Please source the required database table first");
        return;
    }

    QueryResult result = CharacterDatabase.Query("SELECT guid,personality FROM mod_llama_wow_personality");

    if (!result)
    {
        return;
    }
    if (result->GetRowCount() == 0)
    {
        return;
    }    

    if(g_DebugEnabled)
    {
        LOG_INFO("module.mod_llama_wow", "[LlamaWow Chat] Fetching Bot Personality List into array");
    }

    do
    {
        uint64_t personalityBotGUID = result->Fetch()[0].Get<uint64_t>();
        std::string personalityKey = result->Fetch()[1].Get<std::string>();
        g_BotPersonalityList[personalityBotGUID] = personalityKey;
    } while (result->NextRow());

    // llama-wow: локализованные промпты личностей (конвенция AzerothCore: таблица *_locale)
    g_PersonalityPromptsLocale.clear();
    if (QueryResult locResult = CharacterDatabase.Query("SELECT `key`, `Locale`, `prompt` FROM `mod_llama_wow_personality_templates_locale`"))
    {
        do
        {
            std::string locKey    = (*locResult)[0].Get<std::string>();
            std::string locName   = (*locResult)[1].Get<std::string>();
            std::string locPrompt = (*locResult)[2].Get<std::string>();

            uint8 locId = LOCALE_enUS;
            for (uint8 i = 0; i < TOTAL_LOCALES; ++i)
            {
                if (locName == localeNames[i])
                {
                    locId = i;
                    break;
                }
            }

            g_PersonalityPromptsLocale[locKey][locId] = locPrompt;
        } while (locResult->NextRow());
    }
}

std::string GetMultiLineConfigValue(const std::string& configFilePath, const std::string& key)
{
    std::ifstream infile(configFilePath);
    if (!infile) return "";

    std::string line;
    std::string value;
    bool foundKey = false;
    while (std::getline(infile, line))
    {
        std::string trimmed = line;
        trimmed.erase(0, trimmed.find_first_not_of(" \t\r\n"));
        if (trimmed.empty() || trimmed[0] == '#')
            continue;
        size_t pos = trimmed.find('=');
        if (!foundKey && pos != std::string::npos) {
            std::string possibleKey = trimmed.substr(0, pos);
            possibleKey.erase(possibleKey.find_last_not_of(" \t\r\n") + 1);
            if (possibleKey == key) {
                foundKey = true;
                std::string afterEq = trimmed.substr(pos + 1);
                afterEq.erase(0, afterEq.find_first_not_of(" \t\r\n"));
                value += afterEq;
                continue;
            }
        }
        else if (foundKey) {
            // New config key or section
            if (trimmed.find('=') != std::string::npos && trimmed.find('[') == std::string::npos)
                break;
            if (!value.empty()) value += "\n";
            value += trimmed;
        }
    }

    return value;
}

void LoadLlamaWowChatConfig()
{
    g_SayDistance                     = sConfigMgr->GetOption<float>("mod_llama_wow.SayDistance", 30.0f);
    g_YellDistance                    = sConfigMgr->GetOption<float>("mod_llama_wow.YellDistance", 100.0f);
    
    // Load per-channel-type reply chances
    g_PlayerReplyChance_Say           = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.PlayerReplyChance.Say", 90);
    g_BotReplyChance_Say              = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.BotReplyChance.Say", 10);
    g_PlayerReplyChance_Channel       = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.PlayerReplyChance.Channel", 50);
    g_BotReplyChance_Channel          = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.BotReplyChance.Channel", 5);
    g_PlayerReplyChance_Party         = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.PlayerReplyChance.Party", 90);
    g_BotReplyChance_Party            = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.BotReplyChance.Party", 10);
    g_PlayerReplyChance_Guild         = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.PlayerReplyChance.Guild", 70);
    g_BotReplyChance_Guild            = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.BotReplyChance.Guild", 5);
    
    g_MaxBotsToPick                   = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.MaxBotsToPick", 2);
    g_LlamaWowUrl                       = sConfigMgr->GetOption<std::string>("mod_llama_wow.Url", "http://localhost:11434/api/generate");
    g_LlamaWowModel                     = sConfigMgr->GetOption<std::string>("mod_llama_wow.Model", "llama3.2:1b");
    g_LlamaWowNumPredict                = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.NumPredict", 40);
    g_LlamaWowTemperature               = sConfigMgr->GetOption<float>("mod_llama_wow.Temperature", 0.8f);
    g_LlamaWowTopP                      = sConfigMgr->GetOption<float>("mod_llama_wow.TopP", 0.95f);
    g_LlamaWowRepeatPenalty             = sConfigMgr->GetOption<float>("mod_llama_wow.RepeatPenalty", 1.1f);
    g_LlamaWowNumCtx                    = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.NumCtx", 0);
    g_LlamaWowNumThreads                = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.NumThreads", 0);
    g_LlamaWowStop                      = sConfigMgr->GetOption<std::string>("mod_llama_wow.Stop", "");
    g_LlamaWowSystemPrompt              = sConfigMgr->GetOption<std::string>("mod_llama_wow.SystemPrompt", "");
    g_LlamaWowSeed                      = sConfigMgr->GetOption<std::string>("mod_llama_wow.Seed", "");

    // [MRafStudio fork] протокол + токен
    g_LlamaWowApiMode                   = sConfigMgr->GetOption<std::string>("mod_llama_wow.ApiMode", "ollama");
    g_LlamaWowApiKey                    = sConfigMgr->GetOption<std::string>("mod_llama_wow.ApiKey", "");
    g_LlamaWowOpenAiDisableThinking     = sConfigMgr->GetOption<bool>("mod_llama_wow.OpenAiDisableThinking", true);

    g_MaxConcurrentQueries            = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.MaxConcurrentQueries", 0);

    g_Enable                          = sConfigMgr->GetOption<bool>("mod_llama_wow.Enable", true);
    g_DisableRepliesInCombat          = sConfigMgr->GetOption<bool>("mod_llama_wow.DisableRepliesInCombat", true);
    g_EnableRandomChatter             = sConfigMgr->GetOption<bool>("mod_llama_wow.EnableRandomChatter", true);
    g_EnableEventChatter              = sConfigMgr->GetOption<bool>("mod_llama_wow.EnableEventChatter", true);
    g_EnableWhisperReplies            = sConfigMgr->GetOption<bool>("mod_llama_wow.EnableWhisperReplies", false);

    g_DebugEnabled                    = sConfigMgr->GetOption<bool>("mod_llama_wow.DebugEnabled", false);
    g_DebugShowFullPrompt             = sConfigMgr->GetOption<bool>("mod_llama_wow.DebugShowFullPrompt", false);

    g_MinRandomInterval               = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.MinRandomInterval", 45);
    g_MaxRandomInterval               = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.MaxRandomInterval", 180);
    g_RandomChatterRealPlayerDistance = sConfigMgr->GetOption<float>("mod_llama_wow.RandomChatterRealPlayerDistance", 40.0f);
    g_RandomChatterBotCommentChance   = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.RandomChatterBotCommentChance", 25);
    g_RandomChatterMaxBotsPerPlayer   = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.RandomChatterMaxBotsPerPlayer", 2);

    g_EnableGuildRandomAmbientChatter = sConfigMgr->GetOption<bool>("mod_llama_wow.EnableGuildRandomAmbientChatter", true);
    g_GuildRandomChatterChance        = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.GuildRandomChatterChance", 10);

    g_EventChatterRealPlayerDistance = sConfigMgr->GetOption<float>("mod_llama_wow.EventChatterRealPlayerDistance", 40.0f);
    g_EventChatterBotCommentChance   = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.EventChatterBotCommentChance", 15);
    g_EventChatterBotSelfCommentChance = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.EventChatterBotSelfCommentChance", 5);
    g_EventChatterMaxBotsPerPlayer   = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.EventChatterMaxBotsPerPlayer", 2);

    g_EnableRPPersonalities           = sConfigMgr->GetOption<bool>("mod_llama_wow.EnableRPPersonalities", false);

    g_RandomChatterPromptTemplate     = sConfigMgr->GetOption<std::string>("mod_llama_wow.RandomChatterPromptTemplate", "");

    // Load random chatter prompt variations
    std::string variationsStr = sConfigMgr->GetOption<std::string>("mod_llama_wow.RandomChatterPromptVariations", "");
    g_RandomChatterPromptVariations.clear();
    if (!variationsStr.empty())
    {
        std::stringstream ss(variationsStr);
        std::string variation;
        while (std::getline(ss, variation, '|'))
        {
            if (!variation.empty())
            {
                g_RandomChatterPromptVariations.push_back(variation);
            }
        }
    }

    // Load random chatter question variations
    std::string questionsStr = sConfigMgr->GetOption<std::string>("mod_llama_wow.RandomChatterQuestionVariations", "");
    g_RandomChatterQuestionVariations.clear();
    if (!questionsStr.empty())
    {
        std::stringstream ss(questionsStr);
        std::string question;
        while (std::getline(ss, question, '|'))
        {
            if (!question.empty())
            {
                g_RandomChatterQuestionVariations.push_back(question);
            }
        }
    }

    g_EventChatterPromptTemplate     = sConfigMgr->GetOption<std::string>("mod_llama_wow.EventChatterPromptTemplate", "");

    g_ChatPromptTemplate              = sConfigMgr->GetOption<std::string>("mod_llama_wow.ChatPromptTemplate", "");
    
    g_ChatExtraInfoTemplate           = sConfigMgr->GetOption<std::string>("mod_llama_wow.ChatExtraInfoTemplate", "");

    g_DefaultPersonalityPrompt        = sConfigMgr->GetOption<std::string>("mod_llama_wow.DefaultPersonalityPrompt", "");

    g_MaxConversationHistory          = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.MaxConversationHistory", 5);
    g_ConversationHistorySaveInterval = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.ConversationHistorySaveInterval", 10);

    g_ChatHistoryHeaderTemplate       = sConfigMgr->GetOption<std::string>("mod_llama_wow.ChatHistoryHeaderTemplate", "");
    g_ChatHistoryLineTemplate         = sConfigMgr->GetOption<std::string>("mod_llama_wow.ChatHistoryLineTemplate", "");
    g_ChatHistoryFooterTemplate       = sConfigMgr->GetOption<std::string>("mod_llama_wow.ChatHistoryFooterTemplate", "");

    g_EnableChatBotSnapshotTemplate   = sConfigMgr->GetOption<bool>("mod_llama_wow.EnableChatBotSnapshotTemplate", false);
    g_ChatBotSnapshotTemplate         = sConfigMgr->GetOption<std::string>("mod_llama_wow.ChatBotSnapshotTemplate", "");

    g_EnableChatHistory               = sConfigMgr->GetOption<bool>("mod_llama_wow.EnableChatHistory", true);

    // Bot-Player Sentiment Tracking
    g_EnableSentimentTracking         = sConfigMgr->GetOption<bool>("mod_llama_wow.EnableSentimentTracking", true);
    g_SentimentDefaultValue           = sConfigMgr->GetOption<float>("mod_llama_wow.SentimentDefaultValue", 0.5f);
    g_SentimentAdjustmentStrength     = sConfigMgr->GetOption<float>("mod_llama_wow.SentimentAdjustmentStrength", 0.1f);
    g_SentimentSaveInterval           = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.SentimentSaveInterval", 10);
    g_SentimentAnalysisPrompt         = sConfigMgr->GetOption<std::string>("mod_llama_wow.SentimentAnalysisPrompt", "Analyze the sentiment of this message: \"{message}\". Respond only with: POSITIVE, NEGATIVE, or NEUTRAL.");
    g_SentimentPromptTemplate         = sConfigMgr->GetOption<std::string>("mod_llama_wow.SentimentPromptTemplate", "Your relationship sentiment with {player_name} is {sentiment_value} (0.0=hostile, 0.5=neutral, 1.0=friendly). Use this to guide your tone and response.");

    // RAG (Retrieval-Augmented Generation) System
    g_EnableRAG                       = sConfigMgr->GetOption<bool>("mod_llama_wow.EnableRAG", false);
    g_RAGDataPath                     = sConfigMgr->GetOption<std::string>("mod_llama_wow.RAGDataPath", "rag/");
    g_RAGMaxRetrievedItems            = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.RAGMaxRetrievedItems", 3);
    g_RAGSimilarityThreshold          = sConfigMgr->GetOption<float>("mod_llama_wow.RAGSimilarityThreshold", 0.3f);
    g_RAGPromptTemplate               = sConfigMgr->GetOption<std::string>("mod_llama_wow.RAGPromptTemplate", "RELEVANT INFORMATION:\n{rag_info}\nUse this information to provide accurate and detailed responses when applicable.");

    g_ChatterUseGeneralChannel          = sConfigMgr->GetOption<bool>("mod_llama_wow.Chatter.UseGeneralChannel", true);
    g_ChatterUseTradeChannel            = sConfigMgr->GetOption<bool>("mod_llama_wow.Chatter.UseTradeChannel", true);
    g_ChatterUseLfgChannel              = sConfigMgr->GetOption<bool>("mod_llama_wow.Chatter.UseLookingForGroupChannel", false);
    g_ChatterUseGuildRecruitmentChannel = sConfigMgr->GetOption<bool>("mod_llama_wow.Chatter.UseGuildRecruitmentChannel", false);

    // --- Long-term memory and relationships ------------------------------
    g_MemoryEnable                 = sConfigMgr->GetOption<bool>("mod_llama_wow.Memory.Enable", true);
    g_MemoryHistoryTokenLimit      = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.Memory.HistoryTokenLimit", 1500);
    g_MemoryPromptTokenBudget      = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.Memory.PromptTokenBudget", 400);
    g_MemoryMaxPerBot              = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.Memory.MaxPerBot", 40);
    g_MemorySaveInterval           = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.Memory.SaveInterval", 10);

    g_MemoryCondensePrompt         = sConfigMgr->GetOption<std::string>("mod_llama_wow.Memory.CondensePrompt", "");
    if (g_MemoryCondensePrompt.empty())
    {
        g_MemoryCondensePrompt =
            "You are {bot_name}. Below is a record of conversations you took part in.\n"
            "{history}\n"
            "Write the handful of things worth remembering long after the words are forgotten: "
            "what happened, what you learned about someone, how something made you feel. "
            "One per line. Prefix each with an importance from 1 to 10 and a pipe, like "
            "\"7 | ...\". Write them in the third person as short factual notes, at most 20 words each. "
            "Skip small talk. If nothing is worth keeping, write nothing.";
    }

    g_MemoryPromptTemplate         = sConfigMgr->GetOption<std::string>("mod_llama_wow.Memory.PromptTemplate", "");
    if (g_MemoryPromptTemplate.empty())
        g_MemoryPromptTemplate = " Things you remember:\n{memories}";

    g_RelationshipEnable           = sConfigMgr->GetOption<bool>("mod_llama_wow.Relationship.Enable", true);
    g_RelationshipMentionThreshold = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.Relationship.MentionThreshold", 8);
    g_RelationshipMaxPerPrompt     = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.Relationship.MaxPerPrompt", 3);
    g_RelationshipMaxLength        = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.Relationship.MaxLength", 220);

    g_RelationshipUpdatePrompt     = sConfigMgr->GetOption<std::string>("mod_llama_wow.Relationship.UpdatePrompt", "");
    if (g_RelationshipUpdatePrompt.empty())
    {
        g_RelationshipUpdatePrompt =
            "You are {bot_name}. What you currently think of {other_name}: {existing}\n"
            "Recent conversations:\n{history}\n"
            "In one or two sentences, write how you now feel about {other_name} and why. "
            "Revise your earlier view only if these conversations actually changed it. "
            "Write it as a plain statement, no quotes, no preamble.";
    }

    g_RelationshipPromptTemplate   = sConfigMgr->GetOption<std::string>("mod_llama_wow.Relationship.PromptTemplate", "");
    if (g_RelationshipPromptTemplate.empty())
        g_RelationshipPromptTemplate = " How you feel about people you know:\n{relationships}";

    // --- Optional sampling controls --------------------------------------
    // Left unset by default so nothing changes unless an operator opts in.
    g_LlamaWowTopK                      = sConfigMgr->GetOption<int32_t>("mod_llama_wow.TopK", -1);
    g_LlamaWowMinP                      = sConfigMgr->GetOption<float>("mod_llama_wow.MinP", -1.0f);
    g_LlamaWowPresencePenalty           = sConfigMgr->GetOption<float>("mod_llama_wow.PresencePenalty", -1000.0f);
    g_LlamaWowFrequencyPenalty          = sConfigMgr->GetOption<float>("mod_llama_wow.FrequencyPenalty", -1000.0f);

    // --- Think mode ------------------------------------------------------
    // ThinkModeEnableForModule is kept as a deprecated alias: when it is set
    // and ThinkMode is left at auto, it forces think on, matching old configs.
    g_ThinkModeEnableForModule        = sConfigMgr->GetOption<bool>("mod_llama_wow.ThinkModeEnableForModule", false);
    g_ThinkModePolicy                 = static_cast<uint8_t>(LlamaWowCapability_ParsePolicy(
                                            sConfigMgr->GetOption<std::string>("mod_llama_wow.ThinkMode", "auto"),
                                            g_ThinkModeEnableForModule));
    g_ThinkMaxLatencyMs               = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.ThinkMaxLatencyMs", 12000);
    g_ReasoningTokenReserve           = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.ReasoningTokenReserve", 512);
    g_CapabilityProbeTimeoutSeconds   = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.CapabilityProbeTimeoutSeconds", 10);

    // --- HTTP / dispatcher ----------------------------------------------
    g_HttpTimeoutSeconds              = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.HttpTimeoutSeconds", 120);
    g_DispatchWorkerThreads           = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.WorkerThreads",
                                            g_MaxConcurrentQueries > 0 ? g_MaxConcurrentQueries : 4);
    g_MaxQueueDepth                   = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.MaxQueueDepth", 64);

    // --- Response pipeline ----------------------------------------------
    g_MaxReplyLength                  = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.MaxReplyLength", 240);
    g_ResponseStripMarkdown           = sConfigMgr->GetOption<bool>("mod_llama_wow.StripMarkdown", true);
    g_ResponseStripDecorativeUnicode  = sConfigMgr->GetOption<bool>("mod_llama_wow.StripDecorativeUnicode", true);

    // --- Conversation governor -------------------------------------------
    g_MaxChainDepth                   = static_cast<uint8_t>(sConfigMgr->GetOption<uint32_t>("mod_llama_wow.BotConversation.MaxChainDepth", 3));
    g_ChainChanceDecayPct             = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.BotConversation.ChanceDecayPct", 50);
    g_RequireRecentHuman              = sConfigMgr->GetOption<bool>("mod_llama_wow.BotConversation.RequireRecentHuman", true);
    g_HumanWindowSeconds              = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.BotConversation.HumanWindowSeconds", 120);
    g_BotCooldownSeconds              = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.Cooldown.PerBotSeconds", 45);
    g_ScopeCooldownSeconds            = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.Cooldown.PerScopeSeconds", 15);
    g_DirectAddressGroupSize          = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.Cooldown.DirectAddressGroupSize", 5);
    g_ConversationWindowSeconds       = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.Cooldown.ConversationWindowSeconds", 180);
    g_DirectAddressReplyChance        = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.DirectAddressReplyChance", 95);
    g_ScopeMessagesPerMinute          = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.RateLimit.ScopePerMinute", 8);
    g_GlobalMessagesPerMinute         = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.RateLimit.GlobalPerMinute", 40);
    g_BotHistorySize                  = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.Repetition.BotHistorySize", 12);
    g_ScopeHistorySize                = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.Repetition.ScopeHistorySize", 30);
    g_RepetitionSimilarityThreshold   = sConfigMgr->GetOption<float>("mod_llama_wow.Repetition.SimilarityThreshold", 0.72f);
    g_RepetitionWindowSeconds         = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.Repetition.WindowSeconds", 1800);
    g_OpenerHistorySize               = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.Repetition.OpenerHistorySize", 8);

    // --- Topic engine ----------------------------------------------------
    g_TopicWeightPeople               = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.Topic.WeightPeople", 30);
    g_TopicWeightWorld                = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.Topic.WeightWorld", 30);
    g_TopicWeightActivity             = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.Topic.WeightActivity", 25);
    g_TopicWeightSelf                 = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.Topic.WeightSelf", 15);
    g_TopicWeightGuild                = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.Topic.WeightGuild", 20);
    g_TopicMemoryCount                = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.Topic.MemoryCount", 4);
    g_TopicEventMemorySize            = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.Topic.EventMemorySize", 6);
    g_TopicEventMemorySeconds         = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.Topic.EventMemorySeconds", 300);
    g_TopicPlayerRadius               = sConfigMgr->GetOption<float>("mod_llama_wow.Topic.PlayerRadius", 40.0f);
    g_RandomChatterQuestionChance     = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.RandomChatterQuestionChance", 35);

    // --- Snapshot limits -------------------------------------------------
    g_SnapshotIncludeSpells           = sConfigMgr->GetOption<bool>("mod_llama_wow.Snapshot.IncludeSpells", false);
    g_SnapshotMaxSpells               = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.Snapshot.MaxSpells", 6);
    g_SnapshotMaxCreatures            = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.Snapshot.MaxCreatures", 8);
    g_SnapshotMaxObjects              = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.Snapshot.MaxObjects", 8);
    g_SnapshotMaxPlayers              = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.Snapshot.MaxPlayers", 8);

    // --- Roleplay --------------------------------------------------------
    g_RoleplayEnable                  = sConfigMgr->GetOption<bool>("mod_llama_wow.Roleplay.Enable", false);
    g_RoleplayStrictness              = static_cast<uint8_t>(sConfigMgr->GetOption<uint32_t>("mod_llama_wow.Roleplay.Strictness", 1));
    g_RoleplayUseRaceVoice            = sConfigMgr->GetOption<bool>("mod_llama_wow.Roleplay.UseRaceVoice", true);
    g_RoleplayUseClassVoice           = sConfigMgr->GetOption<bool>("mod_llama_wow.Roleplay.UseClassVoice", true);
    g_RoleplayFactionAttitude         = sConfigMgr->GetOption<bool>("mod_llama_wow.Roleplay.FactionAttitude", true);
    g_RoleplayBlockMetaTerms          = sConfigMgr->GetOption<bool>("mod_llama_wow.Roleplay.BlockMetaTerms", true);
    g_RoleplayMetaTermList            = sConfigMgr->GetOption<std::string>("mod_llama_wow.Roleplay.MetaTermList",
        "dps|nerf|proc|patch|server|realm|respec|mmo|npc|toon|irl|lfg|afk|noob|meta|aggro|dot|hot|aoe|wipe|hitbox|hotfix|expansion");
    g_RoleplayCrossFactionGibberish   = sConfigMgr->GetOption<bool>("mod_llama_wow.Roleplay.CrossFactionGibberish", true);

    // --- Embodiment ------------------------------------------------------
    g_EnableBotFacing                 = sConfigMgr->GetOption<bool>("mod_llama_wow.EnableBotFacing", true);
    g_EnableBotEmotes                 = sConfigMgr->GetOption<bool>("mod_llama_wow.EnableBotEmotes", true);
    g_StateEmoteDurationMs            = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.StateEmoteDurationMs", 15000);
    g_GestureChance                   = sConfigMgr->GetOption<int>("mod_llama_wow.GestureChance", 40);

    // Resolved to an id here, on the world thread. The worker that parses a
    // reply must never touch a config string -- reload reassigns them.
    {
        const std::string fallbackName =
            sConfigMgr->GetOption<std::string>("mod_llama_wow.EmoteFallback", "talk");
        g_EmoteFallbackId = fallbackName.empty() ? 0 : LookupTextEmoteId(fallbackName);

        if (!fallbackName.empty() && g_EmoteFallbackId == 0)
        {
            LOG_INFO("module.mod_llama_wow",
                     "[LlamaWow Chat] mod_llama_wow.EmoteFallback '{}' is not a known emote name; "
                     "unresolved gesture tags will play nothing.", fallbackName);
        }
    }
    g_BotExpressionDelayMs            = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.BotExpressionDelayMs", 400);
    g_BotFacingMaxDistance            = sConfigMgr->GetOption<float>("mod_llama_wow.Facing.MaxDistance", 40.0f);
    g_EnableEmoteReactions            = sConfigMgr->GetOption<bool>("mod_llama_wow.EnableEmoteReactions", true);
    g_EmoteReplyChance                = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.EmoteReplyChance", 60);
    g_EmoteReplyMirrorWeight          = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.EmoteReply.MirrorWeight", 55);
    g_EmoteReplyCounterWeight         = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.EmoteReply.CounterWeight", 30);
    g_EmoteReplySpeakWeight           = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.EmoteReply.SpeakWeight", 15);
    g_EmoteReactionCooldownSeconds    = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.EmoteReply.CooldownSeconds", 20);
    g_EmoteReactionPromptTemplate     = sConfigMgr->GetOption<std::string>("mod_llama_wow.EmoteReactionPromptTemplate", "");
    if (g_EmoteReactionPromptTemplate.empty())
    {
        g_EmoteReactionPromptTemplate =
            "You are {bot_name}, a level {bot_level} {bot_race} {bot_class}. "
            "Personality: {bot_personality_name}: {bot_personality}. "
            "{player_name}, a {player_race} {player_class}, just did the '{emote_name}' emote at you. "
            "React out loud, in character, in under 12 words. "
            "No narration, no asterisks, no markdown, no emojis.";
    }

    // Typing Simulation
    g_EnableTypingSimulation          = sConfigMgr->GetOption<bool>("mod_llama_wow.EnableTypingSimulation", false);
    g_TypingSimulationBaseDelay       = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.TypingSimulationBaseDelay", 1000);
    g_TypingSimulationDelayPerChar    = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.TypingSimulationDelayPerChar", 250);
    g_TypingSimulationMaxDelay        = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.TypingSimulationMaxDelay", 8000);

    g_EventTypeDefeated           = sConfigMgr->GetOption<std::string>("mod_llama_wow.EventTypeDefeated", "");
    g_EventTypeDefeatedPlayer     = sConfigMgr->GetOption<std::string>("mod_llama_wow.EventTypeDefeatedPlayer", "");
    g_EventTypePetDefeated        = sConfigMgr->GetOption<std::string>("mod_llama_wow.EventTypePetDefeated", "");
    g_EventTypeGotItem            = sConfigMgr->GetOption<std::string>("mod_llama_wow.EventTypeGotItem", "");
    g_EventTypeDied               = sConfigMgr->GetOption<std::string>("mod_llama_wow.EventTypeDied", "");
    g_EventTypeCompletedQuest     = sConfigMgr->GetOption<std::string>("mod_llama_wow.EventTypeCompletedQuest", "");
    g_EventTypeLearnedSpell       = sConfigMgr->GetOption<std::string>("mod_llama_wow.EventTypeLearnedSpell", "");
    g_EventTypeRequestedDuel      = sConfigMgr->GetOption<std::string>("mod_llama_wow.EventTypeRequestedDuel", "");
    g_EventTypeStartedDueling     = sConfigMgr->GetOption<std::string>("mod_llama_wow.EventTypeStartedDueling", "");
    g_EventTypeWonDuel            = sConfigMgr->GetOption<std::string>("mod_llama_wow.EventTypeWonDuel", "");
    g_EventTypeLeveledUp          = sConfigMgr->GetOption<std::string>("mod_llama_wow.EventTypeLeveledUp", "");
    g_EventTypeAchievement        = sConfigMgr->GetOption<std::string>("mod_llama_wow.EventTypeAchievement", "");
    g_EventTypeUsedObject         = sConfigMgr->GetOption<std::string>("mod_llama_wow.EventTypeUsedObject", "");


    // Load extra blacklist commands from config (comma-separated list)
    std::string extraBlacklist = sConfigMgr->GetOption<std::string>("mod_llama_wow.BlacklistCommands", "");
    if (!extraBlacklist.empty())
    {
        std::vector<std::string> extraList = SplitString(extraBlacklist, ',');
        for (const auto& cmd : extraList)
        {
            g_BlacklistCommands.push_back(cmd);
        }
    }

    // Publish the endpoint settings for worker threads. They must never read
    // the g_LlamaWow* globals directly -- reassigning those here would free the
    // buffers under a worker mid-request.
    LlamaWowConfig_Publish();

    LoadPersonalityTemplatesFromDB();

    // Loads the environment random chatter message templates for each type.
    // Each config option is a pipe-separated list of string templates,
    // using {} as a placeholder for named substitutions.
    // Helper to load a multi-line config option into a std::vector<std::string>
    auto LoadEnvCommentVector = [](const char* key, const std::vector<std::string>& defaults = {}) -> std::vector<std::string>
    {
        std::string val = sConfigMgr->GetOption<std::string>(key, "");
        std::vector<std::string> result;
        std::istringstream iss(val);
        std::string token;
        while (std::getline(iss, token, '|')) { // Split by '|'
            // Trim whitespace from token
            size_t start = token.find_first_not_of(" \t\r\n");
            size_t end = token.find_last_not_of(" \t\r\n");
            if (start != std::string::npos && end != std::string::npos)
                result.push_back(token.substr(start, end - start + 1));
        }
        if (result.empty() && !defaults.empty())
            return defaults;
        return result;
    };

    g_EnvCommentCreature        = LoadEnvCommentVector("mod_llama_wow.EnvCommentCreature", { "" });
    g_EnvCommentGameObject      = LoadEnvCommentVector("mod_llama_wow.EnvCommentGameObject", { "" });
    g_EnvCommentEquippedItem    = LoadEnvCommentVector("mod_llama_wow.EnvCommentEquippedItem", { "" });
    g_EnvCommentBagItem         = LoadEnvCommentVector("mod_llama_wow.EnvCommentBagItem", { "" });
    g_EnvCommentBagItemSell     = LoadEnvCommentVector("mod_llama_wow.EnvCommentBagItemSell", { "" });
    g_EnvCommentSpell           = LoadEnvCommentVector("mod_llama_wow.EnvCommentSpell", { "" });
    g_EnvCommentQuestArea       = LoadEnvCommentVector("mod_llama_wow.EnvCommentQuestArea", { "" });
    g_EnvCommentVendor          = LoadEnvCommentVector("mod_llama_wow.EnvCommentVendor", { "" });
    g_EnvCommentQuestgiver      = LoadEnvCommentVector("mod_llama_wow.EnvCommentQuestgiver", { "" });
    g_EnvCommentBagSlots        = LoadEnvCommentVector("mod_llama_wow.EnvCommentBagSlots", { "" });
    g_EnvCommentDungeon         = LoadEnvCommentVector("mod_llama_wow.EnvCommentDungeon", { "" });
    g_EnvCommentUnfinishedQuest = LoadEnvCommentVector("mod_llama_wow.EnvCommentUnfinishedQuest", { "" });

    // Guild-specific random chatter templates
    g_GuildEnvCommentGuildMember = LoadEnvCommentVector("mod_llama_wow.GuildEnvCommentGuildMember", { "" });
    g_GuildEnvCommentGuildRank = LoadEnvCommentVector("mod_llama_wow.GuildEnvCommentGuildRank", { "" });
    g_GuildEnvCommentGuildBank = LoadEnvCommentVector("mod_llama_wow.GuildEnvCommentGuildBank", { "" });
    g_GuildEnvCommentGuildMOTD = LoadEnvCommentVector("mod_llama_wow.GuildEnvCommentGuildMOTD", { "" });
    g_GuildEnvCommentGuildInfo = LoadEnvCommentVector("mod_llama_wow.GuildEnvCommentGuildInfo", { "" });
    g_GuildEnvCommentGuildOnlineMembers = LoadEnvCommentVector("mod_llama_wow.GuildEnvCommentGuildOnlineMembers", { "" });
    g_GuildEnvCommentGuildRaid = LoadEnvCommentVector("mod_llama_wow.GuildEnvCommentGuildRaid", { "" });
    g_GuildEnvCommentGuildEndgame = LoadEnvCommentVector("mod_llama_wow.GuildEnvCommentGuildEndgame", { "" });
    g_GuildEnvCommentGuildStrategy = LoadEnvCommentVector("mod_llama_wow.GuildEnvCommentGuildStrategy", { "" });
    g_GuildEnvCommentGuildGroup = LoadEnvCommentVector("mod_llama_wow.GuildEnvCommentGuildGroup", { "" });
    g_GuildEnvCommentGuildPvP = LoadEnvCommentVector("mod_llama_wow.GuildEnvCommentGuildPvP", { "" });
    g_GuildEnvCommentGuildCommunity = LoadEnvCommentVector("mod_llama_wow.GuildEnvCommentGuildCommunity", { "" });

    // --- Topic engine comment pools --------------------------------------
    // These carry built-in defaults so the outward-facing topics work even on
    // a server whose .conf predates them. The old pool had no way at all to
    // talk about the people standing next to the bot.
    g_EnvCommentNearbyPlayer = LoadEnvCommentVector("mod_llama_wow.EnvCommentNearbyPlayer", {
        "You notice {player_name}, a {player_race} {player_class}, {player_state} nearby. Say something to or about them.",
        "{player_name} the {player_class} is {player_state} close by. React to seeing them.",
        "A level {player_level} {player_class} called {player_name} is nearby. Comment on them.",
        "You size up {player_name}, a {player_race} {player_class}. Say what you make of them.",
        "{player_name} is {player_state} within earshot. Greet or remark on them.",
        "You spot {player_name} and note they are {player_state}. Say something fitting."
    });

    g_EnvCommentGroupMember = LoadEnvCommentVector("mod_llama_wow.EnvCommentGroupMember", {
        "Your companion {member_name} the {member_class} is {member_state}. Say something to them.",
        "{member_name} is {member_state}. Comment on how your group is doing.",
        "You glance at {member_name}, your {member_class}, and remark on the group's state.",
        "Talk to {member_name} about what the group should do next."
    });

    g_EnvCommentGuildMemberOnline = LoadEnvCommentVector("mod_llama_wow.EnvCommentGuildMemberOnline", {
        "Your guildmate {member_name} the {member_class} is online. Say something to the guild about them.",
        "Greet {member_name} in guild chat, or ask what they are up to.",
        "Mention {member_name}, level {member_level}, to the guild."
    });

    g_EnvCommentRecentEvent = LoadEnvCommentVector("mod_llama_wow.EnvCommentRecentEvent", {
        "You just saw this happen: {event_text}. React to it.",
        "Something you witnessed: {event_text}. Comment on it naturally.",
        "Bring up what just happened: {event_text}.",
        "You are still thinking about this: {event_text}. Say something about it."
    });

    g_EnvCommentNamedNpc = LoadEnvCommentVector("mod_llama_wow.EnvCommentNamedNpc", {
        "{npc_name} the {npc_role} is standing nearby. Say something about them.",
        "You see {npc_name}, a {npc_role}. Remark on what they offer.",
        "Mention {npc_name} the {npc_role} and whether they are worth visiting."
    });

    g_EnvCommentZoneLandmark = LoadEnvCommentVector("mod_llama_wow.EnvCommentZoneLandmark", {
        "You are in {area_name}, part of {zone_name}. Say something about this place.",
        "Remark on the look or feel of {area_name}.",
        "Comment on what {zone_name} is known for.",
        "Say whether {area_name} is worth lingering in."
    });

    g_EnvCommentTimeOfDay = LoadEnvCommentVector("mod_llama_wow.EnvCommentTimeOfDay", {
        "It is {time_of_day}. Remark on the hour.",
        "Comment on it being {time_of_day} and what that means for travelling.",
        "Mention that it is {time_of_day} and how you feel about it."
    });

    g_EnvCommentCorpse = LoadEnvCommentVector("mod_llama_wow.EnvCommentCorpse", {
        "There is a dead {creature_name} on the ground here. React to it.",
        "You see the corpse of a {creature_name}. Say something about the fight that happened.",
        "Remark on the slain {creature_name} nearby."
    });

    g_EnvCommentDanger = LoadEnvCommentVector("mod_llama_wow.EnvCommentDanger", {
        "A {creature_name} is nearby and it is {level_delta} levels above you. Warn or brag.",
        "The {creature_name} here looks dangerous. Say whether you would take it on.",
        "Comment on the threat the {creature_name} poses."
    });

    g_EnvCommentQuestObjective = LoadEnvCommentVector("mod_llama_wow.EnvCommentQuestObjective", {
        "You are partway through '{quest_name}' and still need to {objective}. Mention it.",
        "Talk about your task '{quest_name}' and what is left: {objective}.",
        "Bring up '{quest_name}' and ask for help or advice with it.",
        "Complain or enthuse about '{quest_name}'."
    });

    g_EnvCommentGroupNeed = LoadEnvCommentVector("mod_llama_wow.EnvCommentGroupNeed", {
        "{member_name} is {need}. Say something useful about it.",
        "Point out that {member_name} is {need}.",
        "React to {member_name} being {need}."
    });

    // --- Roleplay-mode variation lists -----------------------------------
    // These replace the shipped out-of-character lists at strictness 2. The
    // defaults ship in-world: no patches, no specs, no forum talk.
    g_RoleplayPromptVariations = LoadEnvCommentVector("mod_llama_wow.Roleplay.PromptVariations", {
        "Remark on the weather or the road ahead.",
        "Mention a rumour you heard in the last town.",
        "Say something about the war and what it has cost.",
        "Complain about the price of supplies.",
        "Speak of someone you have lost.",
        "Mention where you are headed and why.",
        "Say something about your faith or what you believe in.",
        "Comment on the state of the land here.",
        "Talk about a beast or enemy you have fought recently.",
        "Mention hunger, thirst, or how long since you last slept.",
        "Say something about your homeland.",
        "Remark on how far you are from anywhere safe.",
        "Speak of an old debt or grudge.",
        "Mention a piece of your gear and where it came from.",
        "Say something about the people passing through here."
    });

    g_RoleplayQuestionVariations = LoadEnvCommentVector("mod_llama_wow.Roleplay.QuestionVariations", {
        "Ask whether the road ahead is safe.",
        "Ask if anyone has news from the front.",
        "Ask where a person might find work here.",
        "Ask what happened in this place.",
        "Ask whether anyone has seen a particular kind of beast nearby.",
        "Ask where the nearest inn or camp is.",
        "Ask someone where they hail from.",
        "Ask whether the local lord or chieftain is worth trusting.",
        "Ask if anyone needs a hand with something.",
        "Ask what the strangest thing seen around here has been.",
        "Ask whether the water or food here can be trusted.",
        "Ask how long the fighting has been going on."
    });

    // Guild-specific configuration
    g_EnableGuildEventChatter = sConfigMgr->GetOption<bool>("mod_llama_wow.EnableGuildEventChatter", true);
    g_GuildChatterBotCommentChance = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.GuildChatterBotCommentChance", 25);
    g_GuildChatterMaxBotsPerEvent = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.GuildChatterMaxBotsPerEvent", 2);

    // Guild-specific event templates
    g_GuildEventTypeLevelUp = sConfigMgr->GetOption<std::string>("mod_llama_wow.GuildEventTypeLevelUp", "");
    g_GuildEventTypeDungeonComplete = sConfigMgr->GetOption<std::string>("mod_llama_wow.GuildEventTypeDungeonComplete", "");
    g_GuildEventTypeEpicGear = sConfigMgr->GetOption<std::string>("mod_llama_wow.GuildEventTypeEpicGear", "");
    g_GuildEventTypeRareGear = sConfigMgr->GetOption<std::string>("mod_llama_wow.GuildEventTypeRareGear", "");
    g_GuildEventTypeGuildJoin = sConfigMgr->GetOption<std::string>("mod_llama_wow.GuildEventTypeGuildJoin", "");
    g_GuildEventTypeGuildLogin = sConfigMgr->GetOption<std::string>("mod_llama_wow.GuildEventTypeGuildLogin", "");
    g_GuildEventTypeGuildLeave = sConfigMgr->GetOption<std::string>("mod_llama_wow.GuildEventTypeGuildLeave", "");
    g_GuildEventTypeGuildPromotion = sConfigMgr->GetOption<std::string>("mod_llama_wow.GuildEventTypeGuildPromotion", "");
    g_GuildEventTypeGuildDemotion = sConfigMgr->GetOption<std::string>("mod_llama_wow.GuildEventTypeGuildDemotion", "");

    // Load chance variables for normal events
    g_EventTypeDefeated_Chance = sConfigMgr->GetOption<int>("mod_llama_wow.EventTypeDefeated_Chance", 0);
    g_EventTypeDefeatedPlayer_Chance = sConfigMgr->GetOption<int>("mod_llama_wow.EventTypeDefeatedPlayer_Chance", 0);
    g_EventTypePetDefeated_Chance = sConfigMgr->GetOption<int>("mod_llama_wow.EventTypePetDefeated_Chance", 0);
    g_EventTypeGotItem_Chance = sConfigMgr->GetOption<int>("mod_llama_wow.EventTypeGotItem_Chance", 0);
    g_EventTypeDied_Chance = sConfigMgr->GetOption<int>("mod_llama_wow.EventTypeDied_Chance", 0);
    g_EventTypeCompletedQuest_Chance = sConfigMgr->GetOption<int>("mod_llama_wow.EventTypeCompletedQuest_Chance", 0);
    g_EventTypeLearnedSpell_Chance = sConfigMgr->GetOption<int>("mod_llama_wow.EventTypeLearnedSpell_Chance", 0);
    g_EventTypeRequestedDuel_Chance = sConfigMgr->GetOption<int>("mod_llama_wow.EventTypeRequestedDuel_Chance", 0);
    g_EventTypeStartedDueling_Chance = sConfigMgr->GetOption<int>("mod_llama_wow.EventTypeStartedDueling_Chance", 0);
    g_EventTypeWonDuel_Chance = sConfigMgr->GetOption<int>("mod_llama_wow.EventTypeWonDuel_Chance", 0);
    g_EventTypeLeveledUp_Chance = sConfigMgr->GetOption<int>("mod_llama_wow.EventTypeLeveledUp_Chance", 0);
    g_EventTypeAchievement_Chance = sConfigMgr->GetOption<int>("mod_llama_wow.EventTypeAchievement_Chance", 0);
    g_EventTypeUsedObject_Chance = sConfigMgr->GetOption<int>("mod_llama_wow.EventTypeUsedObject_Chance", 0);

    // Load chance variables for guild events
    g_GuildEventTypeEpicGear_Chance = sConfigMgr->GetOption<int>("mod_llama_wow.GuildEventTypeEpicGear_Chance", 0);
    g_GuildEventTypeRareGear_Chance = sConfigMgr->GetOption<int>("mod_llama_wow.GuildEventTypeRareGear_Chance", 0);
    g_GuildEventTypeGuildJoin_Chance = sConfigMgr->GetOption<int>("mod_llama_wow.GuildEventTypeGuildJoin_Chance", 0);
    g_GuildEventTypeGuildLogin_Chance = sConfigMgr->GetOption<int>("mod_llama_wow.GuildEventTypeGuildLogin_Chance", 0);
    g_GuildEventTypeGuildLeave_Chance = sConfigMgr->GetOption<int>("mod_llama_wow.GuildEventTypeGuildLeave_Chance", 0);
    g_GuildEventTypeGuildPromotion_Chance = sConfigMgr->GetOption<int>("mod_llama_wow.GuildEventTypeGuildPromotion_Chance", 0);
    g_GuildEventTypeGuildDemotion_Chance = sConfigMgr->GetOption<int>("mod_llama_wow.GuildEventTypeGuildDemotion_Chance", 0);
    g_GuildEventTypeGuildAchievement_Chance = sConfigMgr->GetOption<int>("mod_llama_wow.GuildEventTypeGuildAchievement_Chance", 0);
    g_GuildEventTypeLevelUp_Chance = sConfigMgr->GetOption<int>("mod_llama_wow.GuildEventTypeLevelUp_Chance", 0);
    g_GuildEventTypeDungeonComplete_Chance = sConfigMgr->GetOption<int>("mod_llama_wow.GuildEventTypeDungeonComplete_Chance", 0);


    // Cooldown time for events
    g_EventCooldownTime = sConfigMgr->GetOption<uint32_t>("mod_llama_wow.EventCooldownTime", 10);

    // Channel disable settings
    g_DisableForCustomChannels = sConfigMgr->GetOption<bool>("mod_llama_wow.DisableForCustomChannels", false);
    g_DisableForSayYell = sConfigMgr->GetOption<bool>("mod_llama_wow.DisableForSayYell", false);
    g_DisableForGuild = sConfigMgr->GetOption<bool>("mod_llama_wow.DisableForGuild", false);
    g_DisableForParty = sConfigMgr->GetOption<bool>("mod_llama_wow.DisableForParty", false);

    LOG_INFO("server.loading",
             "[LlamaWow Chat] Config loaded: Enabled = {}, SayDistance = {}, YellDistance = {}, "
             "Reply Chances - Say: P{}%/B{}%, Channel: P{}%/B{}%, Party: P{}%/B{}%, Guild: P{}%/B{}%, MaxBotsToPick = {}, "
             "Url = {}, Model = {}, MaxConcurrentQueries = {}, EnableRandomChatter = {}, MinRandInt = {}, MaxRandInt = {}, RandomChatterRealPlayerDistance = {}, "
             "RandomChatterBotCommentChance = {}. MaxConcurrentQueries = {}. Extra blacklist commands: {}",
             g_Enable, g_SayDistance, g_YellDistance,
             g_PlayerReplyChance_Say, g_BotReplyChance_Say,
             g_PlayerReplyChance_Channel, g_BotReplyChance_Channel,
             g_PlayerReplyChance_Party, g_BotReplyChance_Party,
             g_PlayerReplyChance_Guild, g_BotReplyChance_Guild,
             g_MaxBotsToPick,
             g_LlamaWowUrl, g_LlamaWowModel, g_MaxConcurrentQueries,
             g_EnableRandomChatter, g_MinRandomInterval, g_MaxRandomInterval, g_RandomChatterRealPlayerDistance,
             g_RandomChatterBotCommentChance, g_MaxConcurrentQueries, extraBlacklist);
}

void LoadPersonalityTemplatesFromDB()
{
    g_PersonalityPrompts.clear();
    g_PersonalityKeys.clear();
    g_PersonalityKeysRandomOnly.clear();

    QueryResult result = CharacterDatabase.Query("SELECT `key`, `prompt`, `manual_only` FROM `mod_llama_wow_personality_templates`");
    if (!result)
    {
        LOG_ERROR("module.mod_llama_wow", "[LlamaWow Chat] No personality templates found in the database!");
        return;
    }

    do
    {
        std::string key = (*result)[0].Get<std::string>();
        std::string prompt = (*result)[1].Get<std::string>();
        bool manualOnly = (*result)[2].Get<bool>();
        
        g_PersonalityPrompts[key] = prompt;
        g_PersonalityKeys.push_back(key);
        
        // Only add to random pool if not manual_only
        if (!manualOnly)
        {
            g_PersonalityKeysRandomOnly.push_back(key);
        }
    } while (result->NextRow());

    LOG_INFO("module.mod_llama_wow", "[LlamaWow Chat] Cached {} personalities ({} available for random assignment).", 
             g_PersonalityKeys.size(), g_PersonalityKeysRandomOnly.size());
}

void LoadBotConversationHistoryFromDB()
{
    QueryResult result = CharacterDatabase.Query(
        "SELECT bot_guid, player_guid, player_message, bot_reply FROM mod_llama_wow_history ORDER BY timestamp ASC"
    );
    if (!result)
        return;

    std::lock_guard<std::mutex> lock(g_ConversationHistoryMutex);
    g_BotConversationHistory.clear();

    do {
        uint64_t botGuid = (*result)[0].Get<uint64_t>();
        uint64_t playerGuid = (*result)[1].Get<uint64_t>();
        std::string playerMsg = (*result)[2].Get<std::string>();
        std::string botReply = (*result)[3].Get<std::string>();

        auto& playerHistory = g_BotConversationHistory[botGuid][playerGuid];
        playerHistory.push_back({ playerMsg, botReply, /*persisted*/ true });
        while (playerHistory.size() > g_MaxConversationHistory)
        {
            playerHistory.pop_front();
        }

    } while (result->NextRow());

}


// Definition of the configuration WorldScript.
LlamaWowChatConfigWorldScript::LlamaWowChatConfigWorldScript() : WorldScript("LlamaWowChatConfigWorldScript") { }

void LlamaWowChatConfigWorldScript::OnStartup()
{
    LoadLlamaWowChatConfig();
    LoadBotPersonalityList();
    LoadBotConversationHistoryFromDB();
    InitializeSentimentTracking();
    Roleplay_Load();
    Memory_Load();

    // Spread the three periodic saves so they do not all come due on the same
    // world tick. Each is incremental now, but they queue onto one database
    // worker and there is no reason to bunch them.
    const time_t nowSeed = time(nullptr);
    g_LastHistorySaveTime   = nowSeed;
    g_LastMemorySaveTime    = nowSeed + static_cast<time_t>(g_MemorySaveInterval) * 60 / 3;
    g_LastSentimentSaveTime = nowSeed + static_cast<time_t>(g_SentimentSaveInterval) * 60 * 2 / 3;

    // Ask LlamaWow what this model can actually do. Runs on its own thread so a
    // missing or slow LlamaWow cannot stall worldserver startup; until it
    // answers, think mode stays off, which is the safe default.
    LlamaWowCapability_Init(true);

    // Bounded worker pool. Replaces one detached std::thread per bot per
    // message, and it is the only place LLM traffic is throttled now.
    LlamaWowDispatch_Start();

    // Initialize RAG system if enabled
    if (g_EnableRAG) {
        if (g_RAGSystem) {
            delete g_RAGSystem;
        }
        g_RAGSystem = new LlamaWowRAGSystem();
        if (!g_RAGSystem->Initialize()) {
            LOG_ERROR("module.mod_llama_wow", "[LlamaWow Chat] Failed to initialize RAG system");
            delete g_RAGSystem;
            g_RAGSystem = nullptr;
        } else {
            LOG_INFO("module.mod_llama_wow", "[LlamaWow Chat] RAG system initialized successfully");
        }
    }
}

void LlamaWowChatConfigWorldScript::OnShutdown()
{
    // Stop accepting work and join the workers before anything they touch goes
    // away. Detached threads had no such guarantee.
    LlamaWowDispatch_Stop();

    SaveBotConversationHistoryToDB();
    Memory_SaveAll();
    if (g_EnableSentimentTracking)
        SaveBotPlayerSentimentsToDB();

    // Clean up RAG system
    if (g_RAGSystem) {
        delete g_RAGSystem;
        g_RAGSystem = nullptr;
        LOG_INFO("module.mod_llama_wow", "[LlamaWow Chat] RAG system cleaned up");
    }
}
