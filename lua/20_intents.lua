-- ════════════════════════════════════════════════════════════════════════════
--  mod-llama-chat: РУССКИЕ ИНТЕНТЫ — игрок пишет фразой, бот выполняет.
--  Работает в world-треде (мгновенно), без внешних прослоек и очередей.
-- ════════════════════════════════════════════════════════════════════════════
local ON_WHISPER = 19
local ON_CHAT    = 18

local function isBot(p)
    if not p then return false end
    local ok, b = pcall(function() return p:IsPlayerbot() end)
    return ok and b
end

-- Найти бота: по имени в тексте, иначе из группы игрока
local function findBot(player, msg)
    local low = string.lower(msg or "")
    local ok, grp = pcall(function() return player:GetGroup() end)
    if ok and grp then
        local ok2, members = pcall(function() return grp:GetMembers() end)
        if ok2 and members then
            for _, m in ipairs(members) do
                if m ~= player and isBot(m) then
                    local ok3, nm = pcall(function() return m:GetName() end)
                    if not (ok3 and nm and string.find(low, string.lower(nm), 1, true)) then
                        return m
                    end
                end
            end
        end
    end
    -- по имени
    if ok and grp then
        local ok2, members = pcall(function() return grp:GetMembers() end)
        if ok2 and members then
            for _, m in ipairs(members) do
                if m ~= player and isBot(m) then
                    local ok3, nm = pcall(function() return m:GetName() end)
                    if ok3 and nm and string.find(low, string.lower(nm), 1, true) then return m end
                end
            end
        end
    end
    return nil
end

local AI = { "подъед", "ко мне", "иди сюда", "следуй", "за мной", "веди" }
local STOP = { "стой", "жди", "замри", "останов" }
local FREE = { "гуляй", "свободен", "иди грайн" }

local function handle(player, msg)
    if not player or not msg or isBot(player) then return end
    local low = string.lower(msg)
    local bot = findBot(player, msg)
    if not bot then return end

    local okAI = pcall(function() return bot:GetBotAI() end)
    if not okAI then return end

    for _, w in ipairs(AI) do
        if string.find(low, w, 1, true) then
            pcall(function()
                bot:GetBotAI():SetMaster(player)
                bot:Teleport(player:GetMapId(), player:GetX() + 1, player:GetY() + 1, player:GetZ(), 0)
                bot:GetBotAI():HandleRemoteCommand("follow")
            end)
            pcall(function() bot:Whisper("Я тут, " .. player:GetName() .. "! Рядом.", 0, player) end)
            return
        end
    end
    for _, w in ipairs(STOP) do
        if string.find(low, w, 1, true) then
            pcall(function() bot:GetBotAI():HandleRemoteCommand("stay") end)
            return
        end
    end
    for _, w in ipairs(FREE) do
        if string.find(low, w, 1, true) then
            pcall(function() bot:GetBotAI():HandleRemoteCommand("grind") end)
            return
        end
    end
end

RegisterPlayerEvent(ON_WHISPER, function(e, player, msg) pcall(handle, player, msg) end)
RegisterPlayerEvent(ON_CHAT,    function(e, player, msg) pcall(handle, player, msg) end)

print("[llama] интенты загружены (русские фразы -> действия бота)")
