#ifndef MOD_LLAMA_WOW_OPENAI_H
#define MOD_LLAMA_WOW_OPENAI_H

// ===========================================================================
//  [MRafStudio fork] OpenAI-совместимый транспорт для mod-llama-wow
// ===========================================================================
//  Зачем: llama.cpp (llama-server), LlamaWow (>=0.2), DeepSeek, LM Studio и
//  прочие говорят на /v1/chat/completions, но НЕ на ollama-нативный
//  /api/generate. Раньше между модулем и llama.cpp стоял внешний
//  Python-мост-переводчик; больше он не нужен.
//
//  Принцип форка: ВСЯ логика OpenAI живёт в этом файле. Основной код
//  патчится микро-вставками (два поля настроек + одна строка ветвления),
//  поэтому обновления upstream мержатся без боли.
//
//  Функции для будущих ролей (изображения/аудио) описаны в
//  mod-llama-wow_openai.cpp как готовые вспомогательные сборщики тел:
//  они не вызываются из модуля-болтовни, но готовы для мультимодальных
//  сценариев (vision-вход от игрока, генерация картинок, TTS/STT).
// ===========================================================================

#include "mod-llama-wow_api.h"

#include <cstdint>
#include <string>

namespace LlamaWowOpenAi
{
    // true, если в конфиге выбран OpenAI-совместимый режим (mod_llama_wow.ApiMode = "openai")
    bool IsEnabled(const LlamaWowEndpointSettings& cfg);

    // Одна генерация через POST /v1/chat/completions.
    // cfg.openAiKey (если не пуст) уходит заголовком "Authorization: Bearer <key>".
    // thinkLevel: "" — думанные токены не запрашиваются (модуль их и не просит).
    LlamaWowApiResult PerformOnce(const LlamaWowEndpointSettings& cfg,
                                const std::string& prompt,
                                const std::string& thinkLevel,
                                uint32_t reasoningReserve);
}

#endif // MOD_LLAMA_WOW_OPENAI_H
