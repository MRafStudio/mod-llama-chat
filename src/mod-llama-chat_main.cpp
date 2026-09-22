#include "mod-llama-chat_config.h"
#include "mod-llama-chat_handler.h"
#include "mod-llama-chat_random.h"
#include "mod-llama-chat_events.h"
#include "mod-llama-chat_command.h"
#include "mod-llama-chat_expression.h"
#include "mod-llama-chat_rag.h"
#include "Log.h"

void Addmod_llama_chatScripts()
{
    LOG_INFO("server.loading", "[Llama Chat] Registering mod-llama-chat scripts.");
    new LlamaChatConfigWorldScript();
    new PlayerBotChatHandler();
    new LlamaChatMaintenance();
    new LlamaBotRandomChatter();

    LOG_INFO("server.loading", "[Llama Chat] Registering mod-llama-chat events.");
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
