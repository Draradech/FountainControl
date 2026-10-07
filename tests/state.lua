local root=assert(arg[1])
local temporary=assert(arg[2])
LoopInGameThreadWithDelay=function() end
FCResetFacilities=function() end
FCGetNativeLimits=function() return false,1000,-1 end
local fingerprint='test-fingerprint'
FCFingerprint=function() return fingerprint end
local warnings={}
FCLog=function(message) warnings[#warnings+1]=message end
local FC=dofile(root..'/Scripts/state.lua')(root)
local subsystem={WorldTitleName={ToString=function()return 'world'end},ReloadedName={ToString=function()return 'SaveGame0'end},
 GetGameSlotFilePath=function()return {ToString=function()return temporary..'/SaveGame0.sav'end}end}
local path=temporary..'/SaveGame0.fountains.json'
local function load(text)
 local file=assert(io.open(path,'wb')); file:write(text); file:close()
 FC.load_settings(subsystem)
 return FC.settings
end
for _,text in ipairs({'invalid','null','42','[]','{}',
 '{"schema":2,"fingerprint":"old","fountains":{"1":{"drop":500}}}',
 '{"schema":1,"fountains":{"1":{"drop":500}}}',
 '{"schema":1,"fingerprint":42,"fountains":{"1":{"drop":500}}}',
 '{"schema":1,"fingerprint":"old","fountains":42}'}) do
 assert(next(load(text))==nil,'Invalid companion must default: '..text)
end
local settings=load('{"schema":1,"fingerprint":"test-fingerprint","fountains":{"1":{"drop":500,"horizontal":250},"2":{"drop":1000,"horizontal":null},"3":{"drop":501,"horizontal":250},"4":{"drop":1050,"horizontal":250},"5":{"drop":500,"horizontal":550},"4294967296":{"drop":500}}}')
assert(settings['1'].drop==500 and settings['1'].horizontal==250)
assert(settings['2'].drop==1000 and settings['2'].horizontal==-1)
assert(settings['3']==nil and settings['4']==nil and settings['5']==nil and settings['4294967296']==nil)
assert(#warnings==0,'Matching or invalid companions must not emit a recovery warning')
local companion=assert(io.open(path,'rb')); local text=companion:read('*a'); companion:close()
for _,changed in ipairs({'later-session-1','later-session-2',''}) do
 fingerprint=changed
 settings=load(text)
 assert(settings['1'].drop==500 and settings['1'].horizontal==250,'Stale companion must restore surviving keys')
 assert(settings['2'].drop==1000 and settings['2'].horizontal==-1)
 assert(settings['3']==nil and settings['4']==nil and settings['5']==nil and settings['4294967296']==nil,'Recovery must retain value validation')
 local warning=warnings[#warnings]
 assert(warning:find('WARNING:',1,true) and warning:find('world/SaveGame0',1,true) and warning:find('persistent SettleKey',1,true))
 assert(warning:find(changed=='' and 'checksum unavailable' or 'checksum mismatch',1,true))
 local existing={GetSettleKey=function()return 1 end}
 local newly_placed={GetSettleKey=function()return 999 end}
 assert(FC.get(existing).drop==500 and FC.get(existing).horizontal==250)
 assert(FC.get(newly_placed).drop==1000 and FC.get(newly_placed).horizontal==-1,'Unrecorded keys must use defaults')
end
assert(#warnings==3,'Each mismatched load must log one warning')
os.remove(path); FC.load_settings(subsystem); assert(next(FC.settings)==nil)
assert(#warnings==3,'Missing companion must not emit a recovery warning')
FCGetNativeLimits=function()return true,350,150 end
local actor={GetSettleKey=function()return 123 end}
assert(FC.get(actor).drop==350 and FC.get(actor).horizontal==150,'Cloned settings must enter Lua state')
print('Companion loading: valid, missing, malformed, repeated stale-save recovery and warnings, unavailable checksum, bounds, increments, clone adoption passed')
