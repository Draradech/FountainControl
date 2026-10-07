local ok, name, remove = pcall(FCNativeInfo)
if not ok then
    print('[FountainControl] Native bridge unavailable: ' .. tostring(name) .. '\n')
    return
end
print(string.format('[FountainControl] %s; RemoveCenterExclusion=%s\n', name, tostring(remove)))
