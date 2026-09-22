CREATE TABLE IF NOT EXISTS mod_llama_chat_bot_player_sentiments (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    bot_guid BIGINT UNSIGNED NOT NULL,
    player_guid BIGINT UNSIGNED NOT NULL,
    sentiment_value FLOAT NOT NULL DEFAULT 0.5 COMMENT 'Sentiment value: 0.0=hostile, 0.5=neutral, 1.0=friendly',
    -- [mod-llama-chat] Четыре оси отношений. Их считает Lua-слой (движок ALE) по весам
    -- из lua-конфига и пишет сюда; правка весов не требует пересборки сервера.
    -- Дробные оси (десятые доли): целых слишком грубо - за три разговора
    -- нельзя становиться другом. Тип DECIMAL(5,2): 0.00 .. 100.00
    trust DECIMAL(5,2) NOT NULL DEFAULT 50.00 COMMENT 'Доверие: 0.00..100.00',
    affection DECIMAL(5,2) NOT NULL DEFAULT 50.00 COMMENT 'Привязанность: 0.00..100.00',
    respect DECIMAL(5,2) NOT NULL DEFAULT 50.00 COMMENT 'Уважение: 0.00..100.00',
    attraction DECIMAL(5,2) NOT NULL DEFAULT 0.00 COMMENT 'Влечение: 0.00..100.00',
    mood VARCHAR(32) NOT NULL DEFAULT 'neutral' COMMENT 'Текущее настроение бота',
    last_reason VARCHAR(128) NOT NULL DEFAULT '' COMMENT 'Последняя причина изменения',
    last_updated DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    -- Unique constraint to ensure one sentiment record per bot-player pair
    UNIQUE KEY unique_sentiment (bot_guid, player_guid),
    -- Indexes for performance
    INDEX idx_bot_guid (bot_guid),
    INDEX idx_player_guid (player_guid),
    INDEX idx_sentiment_value (sentiment_value)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
