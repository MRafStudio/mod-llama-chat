-- ════════════════════════════════════════════════════════════════════════════
--  mod-llama-chat: ОТНОШЕНИЯ (4 оси, дробные значения)
--
--  Схема: игрок пишет в чат -> Lua ловит -> считает по весам из
--  00_relations_weights.lua -> пишет в ТАБЛИЦУ МОДУЛЯ -> модуль читает её
--  и подмешивает чувства в промпт. Один контур, без внешних прослоек.
--
--  Оси хранятся в DECIMAL(5,2): 0.00 .. 100.00 (десятые доли).
-- ════════════════════════════════════════════════════════════════════════════
local ON_WHISPER       = 19
local ON_CHAT          = 18
local ON_KILL_CREATURE = 7

local TABLE = "mod_llama_chat_bot_player_sentiments"

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

local ready = false
local function dbReady()
    if ready then return true end
    local ok, q = pcall(CharDBQuery, "SELECT 1")
    ready = ok and q ~= nil
    return ready
end

-- Анти-лавина: не чаще одного UPDATE на пару бот-игрок за N секунд.
-- Копим изменения в памяти и сбрасываем одной записью.
local pending  = {}
local lastWrite = {}
local теперь = os.time

local function flush(botGuid, playerGuid)
    local key = botGuid .. ":" .. playerGuid
    local acc = pending[key]
    if not acc then return end

    local now = теперь()
    local wait = W.задержка_записи_сек or 5
    if lastWrite[key] and (now - lastWrite[key]) < wait then
        return   -- ещё рано, копим дальше
    end

    CharDBExecute(string.format(
        "INSERT IGNORE INTO `%s` (bot_guid, player_guid, trust, affection, respect, attraction, mood, last_reason) "
        .. "VALUES (%d, %d, 50.00, 50.00, 50.00, 0.00, 'neutral', '')",
        TABLE, botGuid, playerGuid))

    local sets = string.format(
        "trust=LEAST(100.00,GREATEST(0.00,trust+(%.2f))), "
        .. "affection=LEAST(100.00,GREATEST(0.00,affection+(%.2f))), "
        .. "respect=LEAST(100.00,GREATEST(0.00,respect+(%.2f))), "
        .. "attraction=LEAST(100.00,GREATEST(0.00,attraction+(%.2f))), last_reason='%s'",
        acc.trust or 0, acc.affection or 0, acc.respect or 0, acc.attraction or 0, esc(acc.reason))

    if acc.mood then sets = sets .. ", mood='" .. esc(acc.mood) .. "'" end
    -- Производная шкала модуля (0.0..1.0), чтобы штатный sentiment остался осмысленным
    sets = sets .. ", sentiment_value=ROUND(((trust+affection+respect)/300), 4)"

    CharDBExecute(string.format("UPDATE `%s` SET %s WHERE bot_guid=%d AND player_guid=%d",
        TABLE, sets, botGuid, playerGuid))

    pending[key]  = nil
    lastWrite[key] = now
end

-- Прибавить вес: копим, при необходимости пишем
local function apply(botGuid, playerGuid, w)
    if not dbReady() or not w then return false end
    local key = botGuid .. ":" .. playerGuid
    local acc = pending[key] or { trust = 0, affection = 0, respect = 0, attraction = 0 }

    acc.trust      = acc.trust      + (w.trust or 0)
    acc.affection  = acc.affection  + (w.affection or 0)
    acc.respect    = acc.respect    + (w.respect or 0)
    acc.attraction = acc.attraction + (w.attraction or 0)
    if w.mood   then acc.mood = w.mood end
    if w.reason then acc.reason = w.reason end

    pending[key] = acc
    flush(botGuid, playerGuid)
    return true
end

-- Публичная функция для других файлов (дуэли, эмоции, торговля)
function LLAMA_RELATION_APPLY(bot, player, key)
    if not bot or not player or isBot(player) then return false end
    local w = W[key]
    if not w then return false end
    return apply(bot:GetGUIDLow(), player:GetGUIDLow(), w)
end

-- Читает текущие оси (для проверки "друзья ли", вывода, порогов)
function LLAMA_RELATION_GET(botGuid, playerGuid)
    if not dbReady() then return nil end
    local q = CharDBQuery(string.format(
        "SELECT trust, affection, respect, attraction FROM `%s` WHERE bot_guid=%d AND player_guid=%d",
        TABLE, botGuid, playerGuid))
    if not q then return nil end
    return { trust = q:GetFloat(0), affection = q:GetFloat(1), respect = q:GetFloat(2), attraction = q:GetFloat(3) }
end

-- Друзья ли? (пороги в конфиге; уважение не учитываем - можно уважать и врага)
function LLAMA_IS_FRIEND(botGuid, playerGuid)
    local r = LLAMA_RELATION_GET(botGuid, playerGuid)
    if not r then return false end
    local f = (W.ПОРОГИ and W.ПОРОГИ.друг) or { trust = 60.0, affection = 60.0 }
    return r.trust >= (f.trust or 60.0) and r.affection >= (f.affection or 60.0)
end

-- ─── тон общения ────────────────────────────────────────────────────────
local function score(msg)
    local low = string.lower(msg or "")
    local p, n = 0, 0
    for _, word in ipairs(POS) do if string.find(low, word, 1, true) then p = p + 1 end end
    for _, word in ipairs(NEG) do if string.find(low, word, 1, true) then n = n + 1 end end
    return p, n
end

local function scaled(base, k)
    return { trust = (base.trust or 0) * k, affection = (base.affection or 0) * k,
             respect = (base.respect or 0) * k, attraction = (base.attraction or 0) * k,
             mood = base.mood, reason = base.reason }
end

local function onTalk(bot, player, msg)
    if not bot or not player or not msg then return end
    if isBot(player) then return end   -- только живой игрок

    local p, n = score(msg)
    local w
    if n > 0 then
        local k = math.min(1 + (n - 1) * (W.множитель_за_слово or 0.35), W.макс_множитель or 2.0)
        w = scaled(W.разговор_грубый, k)
    elseif p > 0 then
        local k = math.min(1 + (p - 1) * (W.множитель_за_слово or 0.35), W.макс_множитель or 2.0)
        w = scaled(W.разговор_тёплый, k)
    else
        w = W.разговор_обычный
    end
    apply(bot:GetGUIDLow(), player:GetGUIDLow(), w)
end

-- ─── события ────────────────────────────────────────────────────────────
local function onKill(event, player, creature)
    if isBot(player) then
        local ok, master = pcall(function() return player:GetMaster() end)
        if ok and master then pcall(LLAMA_RELATION_APPLY, player, master, "бот_убил_моба") end
    end
end

-- ─── ловим чат ──────────────────────────────────────────────────────────
local function onWhisper(event, player, msg, ctype, lang, receiver)
    if isBot(receiver) then onTalk(receiver, player, msg) end
end

local NEAR = 30.0

local function onChat(event, player, msg, ctype, lang)
    if not player or not msg or isBot(player) then return end
    local low = string.lower(msg)

    -- 1) бот назван по имени
    local q = CharDBQuery("SELECT c.guid, c.name FROM `acore_characters`.`characters` c "
                          .. "JOIN `acore_characters`.`" .. TABLE .. "` r ON r.bot_guid=c.guid WHERE c.online=1")
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

    -- 3) ближайший бот рядом
    local q2 = CharDBQuery("SELECT c.name FROM `acore_characters`.`characters` c "
                           .. "JOIN `acore_characters`.`" .. TABLE .. "` r ON r.bot_guid=c.guid WHERE c.online=1")
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

print("[llama] отношения загружены (4 оси, дробные, таблица " .. TABLE .. ")")
