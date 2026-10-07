local root=assert(arg[1])
local temporary=assert(arg[2])
LoopInGameThreadWithDelay=function() end
FCResetFacilities=function() end
FCGetNativeLimits=function() return false,1000,-1 end
FCFingerprint=function() return 'test-fingerprint' end
local FC=dofile(root..'/Scripts/state.lua')(root)
local subsystem={WorldTitleName={ToString=function()return 'world'end},ReloadedName={ToString=function()return 'SaveGame0'end},
 GetGameSlotFilePath=function()return {ToString=function()return temporary..'/SaveGame0.sav'end}end}
local path=temporary..'/SaveGame0.fountains.json'
local function load(text)
 local file=assert(io.open(path,'wb')); file:write(text); file:close()
 FC.load_settings(subsystem)
 return FC.settings
end
for _,text in ipairs({'invalid','null','42','[]','{}','{"schema":1,"fingerprint":"old","fountains":{"1":{"drop":500}}}'}) do
 assert(next(load(text))==nil,'Invalid companion must default: '..text)
end
local settings=load('{"schema":1,"fingerprint":"test-fingerprint","fountains":{"1":{"drop":500,"horizontal":250},"2":{"drop":1000,"horizontal":null},"3":{"drop":501,"horizontal":250},"4":{"drop":1050,"horizontal":250},"5":{"drop":500,"horizontal":550},"4294967296":{"drop":500}}}')
assert(settings['1'].drop==500 and settings['1'].horizontal==250)
assert(settings['2'].drop==1000 and settings['2'].horizontal==-1)
assert(settings['3']==nil and settings['4']==nil and settings['5']==nil and settings['4294967296']==nil)
os.remove(path); FC.load_settings(subsystem); assert(next(FC.settings)==nil)
FCGetNativeLimits=function()return true,350,150 end
local actor={GetSettleKey=function()return 123 end}
assert(FC.get(actor).drop==350 and FC.get(actor).horizontal==150,'Cloned settings must enter Lua state')
print('Companion loading: valid, missing, malformed, stale, bounds, increments, clone adoption passed')
