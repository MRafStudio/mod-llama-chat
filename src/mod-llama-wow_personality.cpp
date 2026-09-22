#include "mod-llama-wow_personality.h"
#include "Player.h"
#include "PlayerbotMgr.h"
#include "Log.h"
#include "mod-llama-wow_config.h"
#include "DatabaseEnv.h"
#include <random>
#include <vector>

// Internal personality map
std::string GetBotPersonality(Player* bot)
{
    uint64_t botGuid = bot->GetGUID().GetRawValue();

    // If personality already assigned, return it (but only if RP personalities are enabled)
    auto it = g_BotPersonalityList.find(botGuid);
    if (it != g_BotPersonalityList.end())
    {
        // If RP personalities are disabled, reset to default
        if (!g_EnableRPPersonalities)
        {
            g_BotPersonalityList[botGuid] = "default";
            return "default";
        }
        if(g_DebugEnabled)
        {
            LOG_INFO("module.mod_llama_wow", "[LlamaWow Chat] Using existing personality '{}' for bot {}", it->second, bot->GetName());
        }
        return it->second;
    }

    // RP personalities disabled or config not loaded
    if (!g_EnableRPPersonalities || g_PersonalityKeysRandomOnly.empty())
    {
        g_BotPersonalityList[botGuid] = "default";
        return "default";
    }

    // Try to load from database if you have persistence
    if (g_BotPersonalityList.find(botGuid) != g_BotPersonalityList.end())
    {
        // DB stores string keys now
        std::string dbPersonality = g_BotPersonalityList[botGuid];

        if (dbPersonality.empty())
        {
            dbPersonality = "default";
        }

        g_BotPersonalityList[botGuid] = dbPersonality;

        if(g_DebugEnabled)
        {
            LOG_INFO("module.mod_llama_wow", "[LlamaWow Chat] Using database personality '{}' for bot {}", dbPersonality, bot->GetName());
        }
        return dbPersonality;
    }

    // Otherwise, assign randomly from config (only from non-manual personalities)
    uint32 newIdx = urand(0, g_PersonalityKeysRandomOnly.size() - 1);
    std::string chosenPersonality = g_PersonalityKeysRandomOnly[newIdx];
    g_BotPersonalityList[botGuid] = chosenPersonality;

    // Save to database if schema supports string (recommend TEXT or VARCHAR column for personality)
    QueryResult tableExists = CharacterDatabase.Query(
        "SELECT * FROM information_schema.tables WHERE table_schema = 'acore_characters' AND table_name = 'mod_llama_wow_personality' LIMIT 1;");
    if (!tableExists)
    {
        LOG_INFO("module.mod_llama_wow", "[LlamaWow Chat] Please source the required database table first");
    }
    else
    {
        CharacterDatabase.Execute("INSERT INTO mod_llama_wow_personality (guid, personality) VALUES ({}, '{}')", botGuid, chosenPersonality);
    }

    if(g_DebugEnabled)
    {
        LOG_INFO("module.mod_llama_wow", "[LlamaWow Chat] Assigned new personality '{}' to bot {}", chosenPersonality, bot->GetName());
    }
    return chosenPersonality;
}


std::string GetPersonalityPromptAddition(const std::string& personality, uint8 locale)
{
    // llama-wow: сперва локализованный вариант (ruRU и т.д.), затем базовая строка (enUS)
    auto locIt = g_PersonalityPromptsLocale.find(personality);
    if (locIt != g_PersonalityPromptsLocale.end())
    {
        auto varIt = locIt->second.find(locale);
        if (varIt != locIt->second.end() && !varIt->second.empty())
            return varIt->second;
    }

    auto it = g_PersonalityPrompts.find(personality);
    if (it != g_PersonalityPrompts.end())
        return it->second;
    return g_DefaultPersonalityPrompt;
}

bool SetBotPersonality(Player* bot, const std::string& personality)
{
    if (!bot)
        return false;
    
    uint64_t botGuid = bot->GetGUID().GetRawValue();
    
    // Check if personality exists
    if (g_PersonalityPrompts.find(personality) == g_PersonalityPrompts.end() && personality != "default")
    {
        return false;
    }
    
    // Update in memory
    g_BotPersonalityList[botGuid] = personality;
    
    // Update in database
    CharacterDatabase.Execute("REPLACE INTO mod_llama_wow_personality (guid, personality) VALUES ({}, '{}')", 
                             botGuid, personality);
    
    if(g_DebugEnabled)
    {
        LOG_INFO("module.mod_llama_wow", "[LlamaWow Chat] Set personality '{}' for bot {}", personality, bot->GetName());
    }
    
    return true;
}

std::vector<std::string> GetAllPersonalityKeys()
{
    return g_PersonalityKeys;
}

bool PersonalityExists(const std::string& personality)
{
    if (personality == "default")
        return true;
    return g_PersonalityPrompts.find(personality) != g_PersonalityPrompts.end();
}

void ClearAllBotPersonalities()
{
    g_BotPersonalityList.clear();
    if(g_DebugEnabled)
    {
        LOG_INFO("module.mod_llama_wow", "[LlamaWow Chat] Cleared all bot personality assignments due to RP personalities being disabled");
    }
}
