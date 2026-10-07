-- Development-only local mailbox. Disabled in the distributed configuration.
-- Work executes on the game thread; this stays useful for future investigation.
return function(root)
    local function log(message)
        print('[FountainControl:dev] ' .. tostring(message) .. '\n')
        local f = assert(io.open(root .. '/probe.log', 'a'))
        f:write(tostring(message), '\n'); f:close()
    end
    _G.FCDev = {log=log, root=root}
    LoopInGameThreadWithDelay(250, function()
        local f = io.open(root .. '/commands.lua', 'r')
        if not f then return end
        local code = f:read('*a'); f:close()
        os.remove(root .. '/commands.lua')
        local fn, error = load(code, '@FountainControl/commands.lua')
        if not fn then log(error); return end
        local ok, error = pcall(fn)
        if not ok then log(error) end
    end)
    log('Local development commands enabled')
end
