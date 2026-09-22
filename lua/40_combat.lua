-- ════════════════════════════════════════════════════════════════════════════
--  mod-llama-chat: БОЕВОЙ СЛОЙ (экзоскелет) — быстрые реакции в бою.
--
--  Всё работает в world-треде через таймер, без обращений к БД: это тот самый
--  слой "меньше 100 мс" (подтянуть спутника, прикрыть мастера).
-- ════════════════════════════════════════════════════════════════════════════
local TICK_MS   = 500
local NEAR_MASTER = 25.0     -- если бот дальше - подтягиваем
local LOW_HP      = 0.35     -- доля HP мастера, при которой бот встревает

local function isBot(p)
    if not p then return false end
    local ok, b = pcall(function() return p:IsPlayerbot() end)
    return ok and b
end

local function tick()
    -- 1. Спутники держатся рядом с мастером
    local players = GetPlayersInWorld()
    if not players then return end
    for _, bot in ipairs(players) do
        if isBot(bot) and bot:IsInWorld() then
            local ok, ai = pcall(function() return bot:GetBotAI() end)
            if ok and ai then
                local okM, master = pcall(function() return ai:GetMaster() end)
                if okM and master and master:IsInWorld() then
                    local okD, d = pcall(function()
                        local dx = bot:GetX() - master:GetX()
                        local dy = bot:GetY() - master:GetY()
                        local dz = bot:GetZ() - master:GetZ()
                        return math.sqrt(dx*dx + dy*dy + dz*dz)
                    end)
                    if okD and d and d > NEAR_MASTER then
                        pcall(function() ai:HandleRemoteCommand("follow") end)
                    end
                end
            end
        end
    end

    -- 2. Мастер при смерти/низком HP - лечим и прикрываем
    for _, pl in ipairs(players) do
        if not isBot(pl) and pl:IsInWorld() and pl:IsAlive() then
            local ok, hp, maxhp = pcall(function() return pl:GetHealth(), pl:GetMaxHealth() end)
            if ok and hp and maxhp and maxhp > 0 and (hp / maxhp) < LOW_HP then
                local okG, grp = pcall(function() return pl:GetGroup() end)
                if okG and grp then
                    local ok2, members = pcall(function() return grp:GetMembers() end)
                    if ok2 and members then
                        for _, m in ipairs(members) do
                            if m ~= pl and isBot(m) then
                                pcall(function() m:GetBotAI():HandleRemoteCommand("heal") end)
                            end
                        end
                    end
                end
            end
        end
    end
end

CreateLuaEvent(tick, TICK_MS, 0)
print("[llama] боевой слой загружен (тик " .. TICK_MS .. " мс, без обращений к БД)")
