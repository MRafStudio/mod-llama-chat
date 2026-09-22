#ifndef MOD_LLAMA_COMMAND_H
#define MOD_LLAMA_COMMAND_H

#include "ScriptMgr.h"
#include "Chat.h"

class LlamaChatConfigCommand : public CommandScript
{
public:
    LlamaChatConfigCommand();
    Acore::ChatCommands::ChatCommandTable GetCommands() const override;

    static bool HandleLlamaReloadCommand(ChatHandler* handler);
    static bool HandleLlamaSentimentViewCommand(ChatHandler* handler, Optional<std::string> botName, Optional<std::string> playerName);
    static bool HandleLlamaSentimentSetCommand(ChatHandler* handler, std::string botName, std::string playerName, float sentimentValue);
    static bool HandleLlamaSentimentResetCommand(ChatHandler* handler, Optional<std::string> botName, Optional<std::string> playerName);
    static bool HandleLlamaPersonalityGetCommand(ChatHandler* handler, std::string botName);
    static bool HandleLlamaPersonalitySetCommand(ChatHandler* handler, std::string botName, std::string personality);
    static bool HandleLlamaPersonalityListCommand(ChatHandler* handler);

    // Diagnostics. Half of what was wrong with this module was invisible
    // because there was no way to ask it what it thought it was doing.
    static bool HandleLlamaStatusCommand(ChatHandler* handler);
    static bool HandleLlamaTestCommand(ChatHandler* handler, Acore::ChatCommands::Tail prompt);
};

#endif // MOD_LLAMA_COMMAND_H
