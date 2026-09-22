#ifndef MOD_LLAMA_WOW_COMMAND_H
#define MOD_LLAMA_WOW_COMMAND_H

#include "ScriptMgr.h"
#include "Chat.h"

class LlamaWowChatConfigCommand : public CommandScript
{
public:
    LlamaWowChatConfigCommand();
    Acore::ChatCommands::ChatCommandTable GetCommands() const override;

    static bool HandleLlamaWowReloadCommand(ChatHandler* handler);
    static bool HandleLlamaWowSentimentViewCommand(ChatHandler* handler, Optional<std::string> botName, Optional<std::string> playerName);
    static bool HandleLlamaWowSentimentSetCommand(ChatHandler* handler, std::string botName, std::string playerName, float sentimentValue);
    static bool HandleLlamaWowSentimentResetCommand(ChatHandler* handler, Optional<std::string> botName, Optional<std::string> playerName);
    static bool HandleLlamaWowPersonalityGetCommand(ChatHandler* handler, std::string botName);
    static bool HandleLlamaWowPersonalitySetCommand(ChatHandler* handler, std::string botName, std::string personality);
    static bool HandleLlamaWowPersonalityListCommand(ChatHandler* handler);

    // Diagnostics. Half of what was wrong with this module was invisible
    // because there was no way to ask it what it thought it was doing.
    static bool HandleLlamaWowStatusCommand(ChatHandler* handler);
    static bool HandleLlamaWowTestCommand(ChatHandler* handler, Acore::ChatCommands::Tail prompt);
};

#endif // MOD_LLAMA_WOW_COMMAND_H
