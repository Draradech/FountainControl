-- Real native construction from an imported blueprint; disposable worlds only.
-- Set FCBlueprintTestFile to a test PNG filename and FCBlueprintTestPosition to
-- a clear supported location. The test PNG must contain 700/300 and 350/150.
-- Set FCBlueprintConflictKeys to the source keys from that export's world;
-- FCBlueprintRequireCollisions=true requires two actual overlapping keys.
local s=FindFirstOf('GameSaveLoadSubsystem')
assert(s.WorldTitleName:ToString():match('^FountainControl_.*Dev$'))
local pc=FindFirstOf('BeltTDPlayerController')
local state=pc:TryGetControllerState(FName('Clone'))
assert(state:IsValid() and pc:TryEnterState(FName('Clone'),true))
local before={}
local conflict_keys={}
for _,key in ipairs(FCBlueprintConflictKeys or {}) do conflict_keys[key]=true end
local collisions=0
FountainControl.scan()
for _,actor in pairs(FountainControl.actors) do
    before[actor:GetSettleKey()]=true
    -- Deliberate source-ID collisions must not override imported settings.
    if conflict_keys[actor:GetSettleKey()] then
        FountainControl.set(actor,100,50); collisions=collisions+1
    end
end
if FCBlueprintRequireCollisions then assert(collisions==2,'Two actual source-ID collisions required') end
assert(state:EnterBlueprintModePath(s:GetBlueprintsDirectory():ToString(),assert(FCBlueprintTestFile)))
local p=assert(FCBlueprintTestPosition)
local result=FCProbeBlueprintPlace(state:GetAddress(),p[1],p[2],p[3],p[4] or 0,true)
assert(result==2,'Both test fountains must fit at the selected location')
FountainControl.scan()
local settings={}
for _,actor in pairs(FountainControl.actors) do
    if not before[actor:GetSettleKey()] then
        local value=FountainControl.get(actor)
        settings[string.format('%.0f/%.0f',value.drop,value.horizontal)]=true
        FCDev.log(string.format('Imported fountain %u: %.0f / %.0f',actor:GetSettleKey(),value.drop,value.horizontal))
    end
end
local expected=0
for key in pairs(settings) do expected=expected+1 end
assert(expected==2 and settings['700/300'] and settings['350/150'])
FCDev.log('Blueprint limits survived native construction; source-ID collisions exercised: '..collisions)
