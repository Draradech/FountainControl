local ok, name, remove, ready = pcall(FCNativeInfo)
if not ok then
    print('[FountainControl] Native bridge unavailable: ' .. tostring(name) .. '\n')
    return
end
print(string.format('[FountainControl] %s; RemoveCenterExclusion=%s\n', name, tostring(remove)))
local root, development = FCNativeRoot()
if ready then
    local state=dofile(root .. '/Scripts/state.lua')(root)
    dofile(root .. '/Scripts/ui.lua')(state)
end
if development then
    dofile(root .. '/Scripts/dev.lua')(root)
end
