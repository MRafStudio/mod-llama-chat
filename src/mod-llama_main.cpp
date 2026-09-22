#include "mod-llama_config.h"
#include "mod-llama_handler.h"
#include "mod-llama_random.h"
#include "mod-llama_events.h"
#include "mod-llama_command.h"
#include "mod-llama_expression.h"
#include "mod-llama_rag.h"
#include "Log.h"

void Addmod-llamaScripts()
{
    LOG_INFO("server.loading", "[Llama Chat] Registering mod-llama scripts.");
    new LlamaChatConfigWorldScript();
    new PlayerBotChatHandler();
    new LlamaChatMaintenance();
    new LlamaBotRandomChatter();

    LOG_INFO("server.loading", "[Llama Chat] Registering mod-llama events.");
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

    new LlamaChatConfigCommand();
}
