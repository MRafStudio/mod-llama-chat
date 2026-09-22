#include "mod-llama-wow_config.h"
#include "mod-llama-wow_handler.h"
#include "mod-llama-wow_random.h"
#include "mod-llama-wow_events.h"
#include "mod-llama-wow_command.h"
#include "mod-llama-wow_expression.h"
#include "mod-llama-wow_rag.h"
#include "Log.h"

void Addmod-llama-wowScripts()
{
    LOG_INFO("server.loading", "[LlamaWow Chat] Registering mod-llama-wow scripts.");
    new LlamaWowChatConfigWorldScript();
    new PlayerBotChatHandler();
    new LlamaWowChatMaintenance();
    new LlamaWowBotRandomChatter();

    LOG_INFO("server.loading", "[LlamaWow Chat] Registering mod-llama-wow events.");
    new ChatOnKill();
    new ChatOnLoot();
    new ChatOnDeath();
    new ChatOnQuest();
    new ChatOnLearn();
    new ChatOnDuel();
    new ChatOnLevelUp();
    new ChatOnAchievement();
    new ChatOnGameObjectUse();

    // Guild events. ChatOnGuildMemberChange used to exist but was never
    // registered here, and its hooks were not AzerothCore hooks in any case.
    new ChatOnGuild();
    new ChatOnGuildLogin();

    // Bots react when a player emotes at them.
    new ChatOnEmote();

    new LlamaWowChatConfigCommand();
}
