#ifndef MOD_LLAMA_OPENAI_H
#define MOD_LLAMA_OPENAI_H

// ===========================================================================
//  [MRafStudio fork] OpenAI-совместимый транспорт для mod-llama
// ===========================================================================
//  Зачем: llama.cpp (llama-server), Llama (>=0.2), DeepSeek, LM Studio и
//  прочие говорят на /v1/chat/completions, но НЕ на ollama-нативный
//  /api/generate. Раньше между модулем и llama.cpp стоял внешний
//  Python-мост-переводчик; больше он не нужен.
//
//  Принцип форка: ВСЯ логика OpenAI живёт в этом файле. Основной код
//  патчится микро-вставками (два поля настроек + одна строка ветвления),
//  поэтому обновления upstream мержатся без боли.
//
//  Функции для будущих ролей (изображения/аудио) описаны в
//  mod-llama_openai.cpp как готовые вспомогательные сборщики тел:
//  они не вызываются из модуля-болтовни, но готовы для мультимодальных
//  сценариев (vision-вход от игрока, генерация картинок, TTS/STT).
// ===========================================================================

#include "mod-llama_api.h"

#include <cstdint>
#include <string>

namespace LlamaOpenAi
{
    // true, если в конфиге выбран OpenAI-совместимый режим (mod_llama.ApiMode = "openai")
    bool IsEnabled(const LlamaEndpointSettings& cfg);

    // Одна генерация через POST /v1/chat/completions.
    // cfg.openAiKey (если не пуст) уходит заголовком "Authorization: Bearer <key>".
    // thinkLevel: "" — думанные токены не запрашиваются (модуль их и не просит).
    LlamaApiResult PerformOnce(const LlamaEndpointSettings& cfg,
                                const std::string& prompt,
                                const std::string& thinkLevel,
                                uint32_t reasoningReserve);
}

#endif // MOD_LLAMA_OPENAI_H
