-- ════════════════════════════════════════════════════════════════════════════
--  mod-llama-chat: ОТНОШЕНИЯ (4 оси) — единственный контур
--
--  Схема работы:
--     игрок пишет в чат  ->  Lua ловит (ALE)  ->  считает по весам
--     ->  пишет в ТАБЛИЦУ МОДУЛЯ mod_llama_chat_bot_player_sentiments
--     ->  модуль сам читает эту таблицу и подмешивает чувства в промпт.
--
--  Никаких внешних прослоек и "мостов" - всё внутри одного модуля.
--  Математика (веса) лежит в 00_relations_weights.lua и правится на живом
--  сервере: пересборка не нужна.
-- ════════════════════════════════════════════════════════════════════════════
local ON_WHISPER       = 19
local ON_CHAT          = 18
local ON_KILL_CREATURE = 7
local ON_LEVEL_CHANGE  = 13

local TABLE = "mod_llama_chat_bot_player_sentiments"

-- Веса из 00_relations_weights.lua (он загружается первым по имени)
local W = LLAMA_WEIGHTS
if not W then
    print("[llama] FATAL: 00_relations_weights.lua должен загружаться раньше 10_relations.lua")
    return
end

local POS = { "спасибо", "молодец", "умница", "люблю", "любимая", "любимый", "милая", "красив", "нрав",
              "обожаю", "хорош", "класс", "круто", "помогу", "помог", "защищ", "спас", "подар", "друг",
              "подруга", "пожалуйста", "давай", "вместе", "с тобой" }
local NEG = { "дура", "дурак", "идиот", "тупая", "тупой", "убью", "сдохни", "ненавижу", "отврат",
              "мразь", "тупиц", "заткнись", "надоел", "беси", "говно", "дерьмо", "козел", "козёл",
              "сволоч", "пошла вон", "пошёл вон", "молчи" }

-- ─── утилиты ────────────────────────────────────────────────────────────
local function esc(s)
    if not s then return "" end
    return (tostring(s):gsub("'", "''"))
end

local function isBot(p)
    if not p then return false end
    local ok, b = pcall(function() return p:IsPlayerbot() end)
    return ok and b
end

-- Ждём готовности БД (ALE грузится до полного старта мира)
local ready = false
local function dbReady()
    if ready then return true end
    local ok, q = pcall(CharDBQuery, "SELECT 1")
    ready = ok and q ~= nil
    return ready
end

-- ─── запись осей в таблицу модуля ───────────────────────────────────────
-- w = { trust=, affection=, respect=, attraction=, mood=, reason= }
local function apply(botGuid, playerGuid, w)
    if not dbReady() then return false end

    -- 1. Строка должна существовать: создаём при первом контакте
    CharDBExecute(string.format(
        "INSERT IGNORE INTO `%s` (bot_guid, player_guid, trust, affection, respect, attraction, mood, last_reason) "
        .. "VALUES (%d, %d, 50, 50, 50, 0, 'neutral', '')",
        TABLE, botGuid, playerGuid))

    -- 2. Изменяем оси (клампы 0..100 на стороне MySQL, как и раньше)
    local sets = string.format(
        "trust=LEAST(100,GREATEST(0,trust+(%d))), "
        .. "affection=LEAST(100,GREATEST(0,affection+(%d))), "
        .. "respect=LEAST(100,GREATEST(0,respect+(%d))), "
        .. "attraction=LEAST(100,GREATEST(0,attraction+(%d))), last_reason='%s'",
        w.trust or 0, w.affection or 0, w.respect or 0, w.attraction or 0, esc(w.reason))

    if w.mood then sets = sets .. ", mood='" .. esc(w.mood) .. "'" end

    -- 3. Производная шкала модуля (0.0..1.0) - чтобы его собственный sentiment
    --    оставался осмысленным: берём среднее тепла/доверия/уважения.
    sets = sets .. ", sentiment_value=ROUND(((trust+affection+respect)/300), 4)"

    CharDBExecute(string.format("UPDATE `%s` SET %s WHERE bot_guid=%d AND player_guid=%d",
        TABLE, sets, botGuid, playerGuid))
    return true
end

-- ─── тон общения ────────────────────────────────────────────────────────
local function score(msg)
    local low = string.lower(msg or "")
    local p, n = 0, 0
    for _, word in ipairs(POS) do if string.find(low, word, 1, true) then p = p + 1 end end
    for _, word in ipairs(NEG) do if string.find(low, word, 1, true) then n = n + 1 end end
    return p, n
end

local function onTalk(bot, player, msg)
    if not bot or not player or not msg then return end
    -- ведём только для живого игрока (иначе боты зашумят друг другу базу)
    if isBot(player) then return end

    local p, n = score(msg)
    local w

    if n > 0 then
        local k = math.min(n, W.макс_множитель)
        local base = W.разговор_грубый
        w = { trust = base.trust * k, affection = base.affection * k, respect = base.respect * k,
              attraction = base.attraction * k, mood = base.mood, reason = base.reason }
    elseif p > 0 then
        local k = math.min(1 + (p - 1) * W.множитель_за_слово, W.макс_множитель)
        local base = W.разговор_тёплый
        w = { trust = base.trust * k, affection = base.affection * k, respect = base.respect * k,
              attraction = base.attraction * k, mood = base.mood, reason = base.reason }
    else
        w = W.разговор_обычный
    end

    apply(bot:GetGUIDLow(), player:GetGUIDLow(), w)
end

-- ─── события мира ───────────────────────────────────────────────────────
-- Публичная функция: другие Lua-файлы (эмоции, торговля, бой) зовут её,
-- чтобы применить вес по имени события из конфига.
function LLAMA_RELATION_APPLY(bot, player, key)
    if not bot or not player or isBot(player) then return false end
    local w = W[key]
    if not w then return false end
    return apply(bot:GetGUIDLow(), player:GetGUIDLow(), w)
end

local function onKill(event, player, creature)
    if isBot(player) then
        -- бот убил моба: уважение к нему растёт (если рядом есть игрок)
        local _, pl = pcall(function() return player:GetMaster() end)
        if pl then pcall(LLAMA_RELATION_APPLY, player, pl, "бот_убил_моба") end
    end
end

local function onLevel(event, player, oldLevel)
    if isBot(player) then
        local _, pl = pcall(function() return player:GetMaster() end)
        if pl then pcall(LLAMA_RELATION_APPLY, player, pl, "бот_новый_уровень") end
    end
end

-- ─── ловим чат ──────────────────────────────────────────────────────────
local function onWhisper(event, player, msg, ctype, lang, receiver)
    if isBot(receiver) then onTalk(receiver, player, msg) end
end

local NEAR = 40.0

local function onChat(event, player, msg, ctype, lang)
    if not player or not msg or isBot(player) then return end
    local low = string.lower(msg)

    -- 1) бот назван по имени
    local q = CharDBQuery("SELECT c.guid, c.name FROM `acore_characters`.`characters` c "
                          .. "JOIN `acore_characters`.`" .. TABLE .. "` r ON r.bot_guid=c.guid "
                          .. "WHERE c.online=1")
    if q then
        repeat
            local nm = q:GetString(1)
            if nm and nm ~= "" and string.find(low, string.lower(nm), 1, true) then
                local b = GetPlayerByName(nm)
                if isBot(b) then onTalk(b, player, msg) end
                return
            end
        until not q:NextRow()
    end

    -- 2) бот в группе игрока
    local ok, grp = pcall(function() return player:GetGroup() end)
    if ok and grp then
        local ok2, members = pcall(function() return grp:GetMembers() end)
        if ok2 and members then
            for _, m in ipairs(members) do
                if m ~= player and isBot(m) then onTalk(m, player, msg) return end
            end
        end
    end

    -- 3) ближайший бот рядом (тёплые слова услышаны)
    local q2 = CharDBQuery("SELECT c.name FROM `acore_characters`.`characters` c "
                           .. "JOIN `acore_characters`.`" .. TABLE .. "` r ON r.bot_guid=c.guid "
                           .. "WHERE c.online=1")
    if q2 then
        repeat
            local b = GetPlayerByName(q2:GetString(0))
            if isBot(b) then
                local ok3, d = pcall(function()
                    local dx, dy, dz = b:GetX() - player:GetX(), b:GetY() - player:GetY(), b:GetZ() - player:GetZ()
                    return math.sqrt(dx * dx + dy * dy + dz * dz)
                end)
                if ok3 and d and d <= NEAR then onTalk(b, player, msg) return end
            end
        until not q2:NextRow()
    end
end

RegisterPlayerEvent(ON_WHISPER, onWhisper)
RegisterPlayerEvent(ON_CHAT, onChat)
RegisterPlayerEvent(ON_KILL_CREATURE, function(e, p, c) pcall(onKill, e, p, c) end)
RegisterPlayerEvent(ON_LEVEL_CHANGE, function(e, p, o) pcall(onLevel, e, p, o) end)

print("[llama] отношения загружены (4 оси, таблица модуля " .. TABLE .. ")")
