-- ════════════════════════════════════════════════════════════════════════════
--  mod-llama-chat: ДУЭЛИ — спарринг между друзьями это ТРЕНИРОВКА, не агрессия
--
--  Логика:
--    * дуэль с ДРУГОМ   -> крошечная прибавка к мастерству (спарринг_друг)
--    * дуэль с прочими  -> ещё меньше (спарринг), просто знакомство через бой
--    * проиграл другу   -> чуть уважения сопернику (дуэль_проигрыш)
--    * напал ВНЕ дуэли  -> предательство (сильный минус)
--
--  Веса - в 00_relations_weights.lua, правятся без пересборки.
-- ════════════════════════════════════════════════════════════════════════════
local ON_DUEL_REQUEST = 9
local ON_DUEL_START   = 10
local ON_DUEL_END     = 11

local function isBot(p)
    if not p then return false end
    local ok, b = pcall(function() return p:IsPlayerbot() end)
    return ok and b
end

-- Вернуть пару (бот, игрок), если дуэль идёт между ботом и живым игроком
local function pair(a, b)
    if isBot(a) and not isBot(b) then return a, b end
    if isBot(b) and not isBot(a) then return b, a end
    return nil, nil
end

-- Бот и игрок стоят на расстоянии дуэли? (защита от ложных пар)
local function near(a, b, maxDist)
    local ok, d = pcall(function()
        local dx, dy, dz = a:GetX() - b:GetX(), a:GetY() - b:GetY(), a:GetZ() - b:GetZ()
        return math.sqrt(dx * dx + dy * dy + dz * dz)
    end)
    return ok and d and d <= (maxDist or 40.0)
end

-- Кто-то вызвал кого-то на дуэль
RegisterPlayerEvent(ON_DUEL_REQUEST, function(event, target, challenger)
    local bot, player = pair(target, challenger)
    if not bot or not player then
        -- возможен вариант: цели в обратном порядке
        bot, player = pair(challenger, target)
    end
    if bot and player and near(bot, player) then
        -- бот доволен вызовом: дружеский спарринг
        pcall(function()
            bot:Whisper("Давай разомнёмся!", 0, player)
        end)
    end
end)

-- Дуэль началась
RegisterPlayerEvent(ON_DUEL_START, function(event, p1, p2)
    local bot, player = pair(p1, p2)
    if not bot then bot, player = pair(p2, p1) end
    if not bot or not player then return end

    local isFriend = false
    local ok, res = pcall(LLAMA_IS_FRIEND, bot:GetGUIDLow(), player:GetGUIDLow())
    if ok and res then isFriend = true end

    if isFriend then
        pcall(LLAMA_RELATION_APPLY, bot, player, "спарринг_друг")
    else
        pcall(LLAMA_RELATION_APPLY, bot, player, "спарринг")
    end
end)

-- Дуэль закончилась: победитель/проигравший
RegisterPlayerEvent(ON_DUEL_END, function(event, winner, loser, duelType)
    -- Если бот проиграл другу - небольшое уважение сопернику
    if isBot(loser) and not isBot(winner) then
        pcall(LLAMA_RELATION_APPLY, loser, winner, "дуэль_проигрыш")
        pcall(function() loser:Whisper("Хороший бой! Ещё разок?", 0, winner) end)
    end
    -- Если бот выиграл у игрока - просто дружеская реплика (без прибавок:
    -- победа над хозяином не должна растить его же доверие)
    if isBot(winner) and not isBot(loser) then
        pcall(function() winner:Whisper("Отличный бой, ты почти достал меня!", 0, loser) end)
    end
end)

print("[llama] дуэли загружены (спарринг с друзьями = тренировка)")
