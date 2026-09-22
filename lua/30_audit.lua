-- ════════════════════════════════════════════════════════════════════════════
--  mod-llama-chat: АУДИТ — глушение служебных команд, которые игроку не нужны.
--  Раньше писал события в БД; теперь просто пишет в консоль сервера.
-- ════════════════════════════════════════════════════════════════════════════
local ON_CHAT = 18

local function isBot(p)
    if not p then return false end
    local ok, b = pcall(function() return p:IsPlayerbot() end)
    return ok and b
end

RegisterPlayerEvent(ON_CHAT, function(event, player, msg, ctype, lang)
    if isBot(player) then return end
    local low = string.lower(msg or "")
    -- .server / .account / .gm - служебные команды, которые игроку видеть не нужно
    local blocked = { "server ", "account ", "gm ", "help server" }
    for _, w in ipairs(blocked) do
        if string.find(low, w, 1, true) == 1 then
            print("[llama] заблокирована служебная команда от " .. tostring(player:GetName()))
            return false
        end
    end
end)

print("[llama] аудит загружен")
