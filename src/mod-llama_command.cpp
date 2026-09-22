#include "mod-llama_command.h"
#include "mod-llama_config.h"
#include "mod-llama_sentiment.h"
#include "mod-llama_personality.h"
#include "mod-llama_api.h"
#include "mod-llama_capability.h"
#include "mod-llama_dispatch.h"
#include "mod-llama_governor.h"
#include "mod-llama_response.h"
#include "mod-llama_roleplay.h"
#include "mod-llama-utilities.h"
#include "Log.h"
#include "DatabaseEnv.h"
#include <thread>
#include "Chat.h"
#include "Config.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "PlayerbotMgr.h"
#include <fmt/core.h>

using namespace Acore::ChatCommands;

LlamaChatConfigCommand::LlamaChatConfigCommand()
    : CommandScript("LlamaChatConfigCommand")
{
}

ChatCommandTable LlamaChatConfigCommand::GetCommands() const
{
    static ChatCommandTable llamaSentimentCommandTable =
    {
        { "view",  HandleLlamaSentimentViewCommand,  SEC_ADMINISTRATOR, Console::Yes },
        { "set",   HandleLlamaSentimentSetCommand,   SEC_ADMINISTRATOR, Console::Yes },
        { "reset", HandleLlamaSentimentResetCommand, SEC_ADMINISTRATOR, Console::Yes }
    };

    static ChatCommandTable llamaPersonalityCommandTable =
    {
        { "get",  HandleLlamaPersonalityGetCommand,  SEC_ADMINISTRATOR, Console::Yes },
        { "set",  HandleLlamaPersonalitySetCommand,  SEC_ADMINISTRATOR, Console::Yes },
        { "list", HandleLlamaPersonalityListCommand, SEC_ADMINISTRATOR, Console::Yes }
    };

    static ChatCommandTable llamaReloadCommandTable =
    {
        { "reload",      HandleLlamaReloadCommand,  SEC_ADMINISTRATOR, Console::Yes },
        { "status",      HandleLlamaStatusCommand,  SEC_ADMINISTRATOR, Console::Yes },
        { "test",        HandleLlamaTestCommand,    SEC_ADMINISTRATOR, Console::Yes },
        { "sentiment",   llamaSentimentCommandTable },
        { "personality", llamaPersonalityCommandTable }
    };

    static ChatCommandTable commandTable =
    {
        { "ollama", llamaReloadCommandTable }
    };

    return commandTable;
}

bool LlamaChatConfigCommand::HandleLlamaReloadCommand(ChatHandler* handler)
{
    sConfigMgr->Reload();
    LoadLlamaChatConfig();
    Roleplay_Load();

    // Re-probe: the operator may have just pointed the module at a different
    // model, and think-mode support is per-model.
    LlamaCapability_Init(true);

    // Clear personality assignments if RP personalities are disabled
    // This ensures that when re-enabled later, bots get fresh random assignments
    if (!g_EnableRPPersonalities)
    {
        ClearAllBotPersonalities();
    }

    LoadBotPersonalityList();
    LoadBotConversationHistoryFromDB();
    InitializeSentimentTracking();
    handler->SendSysMessage("LlamaChat: Configuration reloaded from conf!");
    return true;
}

bool LlamaChatConfigCommand::HandleLlamaSentimentViewCommand(ChatHandler* handler, Optional<std::string> botName, Optional<std::string> playerName)
{
    if (!g_EnableSentimentTracking)
    {
        handler->SendSysMessage("LlamaChat: Sentiment tracking is disabled.");
        return true;
    }

    if (!botName && !playerName)
    {
        // Show all sentiment data
        std::lock_guard<std::mutex> lock(g_SentimentMutex);
        if (g_BotPlayerSentiments.empty())
        {
            handler->SendSysMessage("LlamaChat: No sentiment data found.");
            return true;
        }

        handler->SendSysMessage("LlamaChat: All sentiment data:");
        for (const auto& [botGuid, playerMap] : g_BotPlayerSentiments)
        {
            Player* bot = ObjectAccessor::FindPlayer(ObjectGuid(botGuid));
            std::string botNameStr = bot ? bot->GetName() : std::to_string(botGuid);
            
            for (const auto& [playerGuid, sentiment] : playerMap)
            {
                Player* player = ObjectAccessor::FindPlayer(ObjectGuid(playerGuid));
                std::string playerNameStr = player ? player->GetName() : std::to_string(playerGuid);
                
                handler->SendSysMessage(fmt::format("  Bot '{}' -> Player '{}': {:.3f}", 
                                        botNameStr, playerNameStr, sentiment));
            }
        }
        return true;
    }

    // Find specific bot or player
    Player* targetBot = nullptr;
    Player* targetPlayer = nullptr;

    if (botName)
    {
        targetBot = ObjectAccessor::FindPlayerByName(*botName);
        if (!targetBot)
        {
            handler->SendSysMessage(fmt::format("LlamaChat: Bot '{}' not found.", *botName));
            return true;
        }
        if (!PlayerbotsMgr::instance().GetPlayerbotAI(targetBot))
        {
            handler->SendSysMessage(fmt::format("LlamaChat: Player '{}' is not a bot.", *botName));
            return true;
        }
    }

    if (playerName)
    {
        targetPlayer = ObjectAccessor::FindPlayerByName(*playerName);
        if (!targetPlayer)
        {
            handler->SendSysMessage(fmt::format("LlamaChat: Player '{}' not found.", *playerName));
            return true;
        }
    }

    // Show sentiment for specific bot-player pair or all pairs involving a specific bot/player
    if (targetBot && targetPlayer)
    {
        float sentiment = GetBotPlayerSentiment(targetBot->GetGUID().GetRawValue(), targetPlayer->GetGUID().GetRawValue());
        handler->SendSysMessage(fmt::format("LlamaChat: Bot '{}' -> Player '{}': {:.3f}", 
                                targetBot->GetName(), targetPlayer->GetName(), sentiment));
    }
    else if (targetBot)
    {
        // Show all sentiments for this bot
        uint64_t botGuid = targetBot->GetGUID().GetRawValue();
        std::lock_guard<std::mutex> lock(g_SentimentMutex);
        
        auto botIt = g_BotPlayerSentiments.find(botGuid);
        if (botIt == g_BotPlayerSentiments.end() || botIt->second.empty())
        {
            handler->SendSysMessage(fmt::format("LlamaChat: No sentiment data found for bot '{}'.", targetBot->GetName()));
            return true;
        }

        handler->SendSysMessage(fmt::format("LlamaChat: Sentiment data for bot '{}':", targetBot->GetName()));
        for (const auto& [playerGuid, sentiment] : botIt->second)
        {
            Player* player = ObjectAccessor::FindPlayer(ObjectGuid(playerGuid));
            std::string playerNameStr = player ? player->GetName() : std::to_string(playerGuid);
            handler->SendSysMessage(fmt::format("  -> Player '{}': {:.3f}", playerNameStr, sentiment));
        }
    }
    else if (targetPlayer)
    {
        // Show all sentiments involving this player
        uint64_t playerGuid = targetPlayer->GetGUID().GetRawValue();
        std::lock_guard<std::mutex> lock(g_SentimentMutex);
        
        bool found = false;
        handler->SendSysMessage(fmt::format("LlamaChat: Sentiment data involving player '{}':", targetPlayer->GetName()));
        
        for (const auto& [botGuid, playerMap] : g_BotPlayerSentiments)
        {
            auto playerIt = playerMap.find(playerGuid);
            if (playerIt != playerMap.end())
            {
                Player* bot = ObjectAccessor::FindPlayer(ObjectGuid(botGuid));
                std::string botNameStr = bot ? bot->GetName() : std::to_string(botGuid);
                handler->SendSysMessage(fmt::format("  Bot '{}' -> {:.3f}", botNameStr, playerIt->second));
                found = true;
            }
        }
        
        if (!found)
        {
            handler->SendSysMessage(fmt::format("LlamaChat: No sentiment data found involving player '{}'.", targetPlayer->GetName()));
        }
    }

    return true;
}

bool LlamaChatConfigCommand::HandleLlamaSentimentSetCommand(ChatHandler* handler, std::string botName, std::string playerName, float sentimentValue)
{
    if (!g_EnableSentimentTracking)
    {
        handler->SendSysMessage("LlamaChat: Sentiment tracking is disabled.");
        return true;
    }

    Player* bot = ObjectAccessor::FindPlayerByName(botName);
    if (!bot)
    {
        handler->SendSysMessage(fmt::format("LlamaChat: Bot '{}' not found.", botName));
        return true;
    }
    if (!PlayerbotsMgr::instance().GetPlayerbotAI(bot))
    {
        handler->SendSysMessage(fmt::format("LlamaChat: Player '{}' is not a bot.", botName));
        return true;
    }

    Player* player = ObjectAccessor::FindPlayerByName(playerName);
    if (!player)
    {
        handler->SendSysMessage(fmt::format("LlamaChat: Player '{}' not found.", playerName));
        return true;
    }

    if (sentimentValue < 0.0f || sentimentValue > 1.0f)
    {
        handler->SendSysMessage("LlamaChat: Sentiment value must be between 0.0 and 1.0.");
        return true;
    }

    SetBotPlayerSentiment(bot->GetGUID().GetRawValue(), player->GetGUID().GetRawValue(), sentimentValue);
    handler->SendSysMessage(fmt::format("LlamaChat: Set sentiment between bot '{}' and player '{}' to {:.3f}.", 
                            botName, playerName, sentimentValue));
    return true;
}

bool LlamaChatConfigCommand::HandleLlamaSentimentResetCommand(ChatHandler* handler, Optional<std::string> botName, Optional<std::string> playerName)
{
    if (!g_EnableSentimentTracking)
    {
        handler->SendSysMessage("LlamaChat: Sentiment tracking is disabled.");
        return true;
    }

    if (!botName && !playerName)
    {
        // Reset all sentiment data
        std::lock_guard<std::mutex> lock(g_SentimentMutex);
        uint32_t count = 0;
        for (const auto& [botGuid, playerMap] : g_BotPlayerSentiments)
        {
            count += playerMap.size();
        }
        g_BotPlayerSentiments.clear();
        g_DirtySentiments.clear();
        CharacterDatabase.Execute("DELETE FROM mod_llama_bot_player_sentiments");
        handler->SendSysMessage(fmt::format("LlamaChat: Reset all sentiment data ({} records).", count));
        return true;
    }

    Player* targetBot = nullptr;
    Player* targetPlayer = nullptr;

    if (botName)
    {
        targetBot = ObjectAccessor::FindPlayerByName(*botName);
        if (!targetBot)
        {
            handler->SendSysMessage(fmt::format("LlamaChat: Bot '{}' not found.", *botName));
            return true;
        }
        if (!PlayerbotsMgr::instance().GetPlayerbotAI(targetBot))
        {
            handler->SendSysMessage(fmt::format("LlamaChat: Player '{}' is not a bot.", *botName));
            return true;
        }
    }

    if (playerName)
    {
        targetPlayer = ObjectAccessor::FindPlayerByName(*playerName);
        if (!targetPlayer)
        {
            handler->SendSysMessage(fmt::format("LlamaChat: Player '{}' not found.", *playerName));
            return true;
        }
    }

    if (targetBot && targetPlayer)
    {
        // Reset specific bot-player sentiment
        SetBotPlayerSentiment(targetBot->GetGUID().GetRawValue(), targetPlayer->GetGUID().GetRawValue(), g_SentimentDefaultValue);
        handler->SendSysMessage(fmt::format("LlamaChat: Reset sentiment between bot '{}' and player '{}' to default ({:.3f}).", 
                                targetBot->GetName(), targetPlayer->GetName(), g_SentimentDefaultValue));
    }
    else if (targetBot)
    {
        // Reset all sentiments for this bot
        uint64_t botGuid = targetBot->GetGUID().GetRawValue();
        std::lock_guard<std::mutex> lock(g_SentimentMutex);
        
        auto botIt = g_BotPlayerSentiments.find(botGuid);
        if (botIt != g_BotPlayerSentiments.end())
        {
            uint32_t count = botIt->second.size();
            g_BotPlayerSentiments.erase(botIt);

            for (auto it = g_DirtySentiments.begin(); it != g_DirtySentiments.end(); )
            {
                if (it->first == botGuid)
                    it = g_DirtySentiments.erase(it);
                else
                    ++it;
            }

            CharacterDatabase.Execute(SafeFormat(
                "DELETE FROM mod_llama_bot_player_sentiments WHERE bot_guid = {}", botGuid));
            handler->SendSysMessage(fmt::format("LlamaChat: Reset all sentiment data for bot '{}' ({} records).", 
                                    targetBot->GetName(), count));
        }
        else
        {
            handler->SendSysMessage(fmt::format("LlamaChat: No sentiment data found for bot '{}'.", targetBot->GetName()));
        }
    }
    else if (targetPlayer)
    {
        // Reset all sentiments involving this player
        uint64_t playerGuid = targetPlayer->GetGUID().GetRawValue();
        std::lock_guard<std::mutex> lock(g_SentimentMutex);
        
        uint32_t count = 0;
        for (auto& [botGuid, playerMap] : g_BotPlayerSentiments)
        {
            auto playerIt = playerMap.find(playerGuid);
            if (playerIt != playerMap.end())
            {
                playerMap.erase(playerIt);
                count++;
            }
        }

        for (auto it = g_DirtySentiments.begin(); it != g_DirtySentiments.end(); )
        {
            if (it->second == playerGuid)
                it = g_DirtySentiments.erase(it);
            else
                ++it;
        }

        CharacterDatabase.Execute(SafeFormat(
            "DELETE FROM mod_llama_bot_player_sentiments WHERE player_guid = {}", playerGuid));

        handler->SendSysMessage(fmt::format("LlamaChat: Reset all sentiment data involving player '{}' ({} records).", 
                                targetPlayer->GetName(), count));
    }

    return true;
}

bool LlamaChatConfigCommand::HandleLlamaPersonalityGetCommand(ChatHandler* handler, std::string botName)
{
    Player* bot = ObjectAccessor::FindPlayerByName(botName);
    if (!bot)
    {
        handler->SendSysMessage(fmt::format("LlamaChat: Bot '{}' not found.", botName));
        return true;
    }
    
    if (!PlayerbotsMgr::instance().GetPlayerbotAI(bot))
    {
        handler->SendSysMessage(fmt::format("LlamaChat: Player '{}' is not a bot.", botName));
        return true;
    }
    
    std::string personality = GetBotPersonality(bot);
    std::string prompt = GetPersonalityPromptAddition(personality, handler->GetSessionDbcLocale());
    
    handler->SendSysMessage(fmt::format("LlamaChat: Bot '{}' has personality '{}'", botName, personality));
    handler->SendSysMessage(fmt::format("  Prompt: {}", prompt));
    
    return true;
}

bool LlamaChatConfigCommand::HandleLlamaPersonalitySetCommand(ChatHandler* handler, std::string botName, std::string personality)
{
    Player* bot = ObjectAccessor::FindPlayerByName(botName);
    if (!bot)
    {
        handler->SendSysMessage(fmt::format("LlamaChat: Bot '{}' not found.", botName));
        return true;
    }
    
    if (!PlayerbotsMgr::instance().GetPlayerbotAI(bot))
    {
        handler->SendSysMessage(fmt::format("LlamaChat: Player '{}' is not a bot.", botName));
        return true;
    }
    
    if (!PersonalityExists(personality))
    {
        handler->SendSysMessage(fmt::format("LlamaChat: Personality '{}' does not exist. Use '.llama personality list' to see available personalities.", personality));
        return true;
    }
    
    if (SetBotPersonality(bot, personality))
    {
        std::string prompt = GetPersonalityPromptAddition(personality, handler->GetSessionDbcLocale());
        handler->SendSysMessage(fmt::format("LlamaChat: Set bot '{}' personality to '{}'", botName, personality));
        handler->SendSysMessage(fmt::format("  Prompt: {}", prompt));
    }
    else
    {
        handler->SendSysMessage(fmt::format("LlamaChat: Failed to set personality for bot '{}'.", botName));
    }
    
    return true;
}

bool LlamaChatConfigCommand::HandleLlamaPersonalityListCommand(ChatHandler* handler)
{
    std::vector<std::string> personalities = GetAllPersonalityKeys();
    
    if (personalities.empty())
    {
        handler->SendSysMessage("LlamaChat: No personalities loaded.");
        return true;
    }
    
    handler->SendSysMessage(fmt::format("LlamaChat: Available personalities ({} total, {} random-assignable):", 
                            personalities.size(), g_PersonalityKeysRandomOnly.size()));
    
    for (const auto& personality : personalities)
    {
        std::string prompt = GetPersonalityPromptAddition(personality, handler->GetSessionDbcLocale());
        
        // Check if this personality is manual-only
        bool isManualOnly = (std::find(g_PersonalityKeysRandomOnly.begin(), g_PersonalityKeysRandomOnly.end(), personality) 
                            == g_PersonalityKeysRandomOnly.end());
        
        std::string manualTag = isManualOnly ? " [MANUAL ONLY]" : "";
        
        handler->SendSysMessage(fmt::format("  - {}{}", personality, manualTag));
        handler->SendSysMessage(fmt::format("    {}", prompt));
    }
    
    return true;
}


// ---------------------------------------------------------------------------
// Diagnostics
// ---------------------------------------------------------------------------

bool LlamaChatConfigCommand::HandleLlamaStatusCommand(ChatHandler* handler)
{
    const LlamaDispatchStats dispatch = LlamaDispatch_GetStats();
    const GovernorStats       gov      = Governor_GetStats();

    handler->PSendSysMessage("|cff00ff00[Llama Chat] Status|r");
    handler->PSendSysMessage("Module: {}   Endpoint: {}   Model: {}",
                             g_Enable ? "enabled" : "DISABLED",
                             g_LlamaUrl, g_LlamaModel);
    handler->PSendSysMessage("Think: {}", LlamaCapability_StatusText());

    handler->PSendSysMessage("Dispatcher: {} workers, {} queued, {} in flight, {} awaiting delivery",
                             dispatch.workers, dispatch.queuedRequests,
                             dispatch.inFlight, dispatch.pendingDeliveries);
    handler->PSendSysMessage("Totals: {} submitted, {} delivered, {} failed",
                             (unsigned long long)dispatch.totalSubmitted,
                             (unsigned long long)dispatch.totalDelivered,
                             (unsigned long long)dispatch.totalFailed);
    handler->PSendSysMessage("Dropped: {} queue-full, {} empty-after-cleanup, {} by governor",
                             (unsigned long long)dispatch.totalDroppedQueueFull,
                             (unsigned long long)dispatch.totalDroppedEmpty,
                             (unsigned long long)dispatch.totalDroppedGovernor);

    handler->PSendSysMessage("Governor: {} bots, {} scopes tracked, {} sends in the last minute",
                             gov.trackedBots, gov.trackedScopes, gov.sendsLastMinute);
    handler->PSendSysMessage("Blocked: {} cooldown, {} rate, {} repetition, {} chain-depth, {} no-audience",
                             gov.blockedCooldown, gov.blockedRate, gov.blockedRepetition,
                             gov.blockedChainDepth, gov.blockedNoAudience);

    handler->PSendSysMessage("Roleplay: {} (strictness {})   Emote reactions: {}",
                             g_RoleplayEnable ? "on" : "off",
                             (uint32)g_RoleplayStrictness,
                             g_EnableEmoteReactions ? "on" : "off");
    handler->PSendSysMessage("Topic weights: people {} / world {} / activity {} / self {} / guild {}",
                             g_TopicWeightPeople, g_TopicWeightWorld, g_TopicWeightActivity,
                             g_TopicWeightSelf, g_TopicWeightGuild);

    if (!dispatch.lastError.empty())
        handler->PSendSysMessage("|cffff0000Last error:|r {}", dispatch.lastError);
    else
        handler->PSendSysMessage("Last error: none");

    return true;
}

bool LlamaChatConfigCommand::HandleLlamaTestCommand(ChatHandler* handler, Acore::ChatCommands::Tail prompt)
{
    std::string text(prompt);
    if (text.empty())
    {
        handler->SendSysMessage("Usage: .llama test <prompt>");
        handler->SetSentErrorMessage(true);
        return false;
    }

    handler->PSendSysMessage("[Llama Chat] Sending test prompt, please wait...");

    // Blocking HTTP must not run on the world thread, so do the round trip on
    // a scratch thread and report from there. Turns "the bots are quiet" into
    // a one-command diagnosis: you see the raw output and the cleaned output
    // side by side.
    std::thread([text]()
    {
        LlamaApiResult api = QueryLlama(text, LlamaRequestKind::ChatReply);

        if (!api.ok)
        {
            LOG_INFO("module.mod_llama", "[Llama Chat] TEST FAILED after {}ms: {}",
                     api.latencyMs, api.error.empty() ? "unknown error" : api.error);
            return;
        }

        uint32_t emote = 0;
        const std::string cleaned = ProcessLlmResponse(api.text, "Tester", &emote);

        LOG_INFO("module.mod_llama", "[Llama Chat] TEST ok in {}ms (think={}).",
                 api.latencyMs, api.thinkUsed ? "yes" : "no");
        LOG_INFO("module.mod_llama", "[Llama Chat] TEST raw     : {}", api.text);
        LOG_INFO("module.mod_llama", "[Llama Chat] TEST cleaned : {}", cleaned);
        if (emote)
            LOG_INFO("module.mod_llama", "[Llama Chat] TEST emote   : {}", emote);
        if (!api.thinking.empty())
            LOG_INFO("module.mod_llama", "[Llama Chat] TEST thinking: {}", api.thinking);
    }).detach();

    handler->PSendSysMessage("[Llama Chat] Result will appear in the server log (module.mod_llama).");
    return true;
}
