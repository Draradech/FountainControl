-- Development-only local mailbox. Disabled in the distributed configuration.
-- Work executes on the game thread; this stays useful for future investigation.
return function(root)
    local function log(message)
        print('[FountainControl:dev] ' .. tostring(message) .. '\n')
        local f = assert(io.open(root .. '/probe.log', 'a'))
        f:write(tostring(message), '\n'); f:close()
    end
    _G.FCDev = {log=log, root=root}
    LoopAsync(250, function()
        local f = io.open(root .. '/commands.lua', 'r')
        if not f then return end
        local code = f:read('*a'); f:close()
        os.remove(root .. '/commands.lua')
        ExecuteInGameThread(function()
            local fn, error = load(code, '@FountainControl/commands.lua')
            if not fn then log(error); return end
            local ok, error = pcall(fn)
            if not ok then log(error) end
        end, EGameThreadMethod.EngineTick)
    end)
    local function hook(path, callback)
        local ok, error = pcall(RegisterHook, path, callback)
        if not ok then log('Cannot hook ' .. path .. ': ' .. tostring(error)) end
    end
    hook('/Script/BeltTD.BeltTDActivatableWidget:SetContextActor', function(context, actor)
        log('UI context: ' .. context:get():GetFullName() .. ' actor=' .. actor:get():GetFullName())
    end)
    log('Local development commands enabled')
end
