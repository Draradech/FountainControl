-- Read-only validation while actual copy/move/blueprint ghosts are active.
-- Optionally set FCPreviewExpected to a list of expected horizontal caps in cm.
local s=FindFirstOf('GameSaveLoadSubsystem')
assert(s.WorldTitleName:ToString():match('^FountainControl_.*Dev$'))
local pc=FindFirstOf('BeltTDPlayerController')
FountainControl.preview()
local limits=FCGetGhostPreviews(pc:GetAddress())
local observed={};local count=0
for _,actor in ipairs(FindAllOf('BP_SmartCannon_C') or {}) do
    local h=limits[actor:GetAddress()]
    if h~=nil then
        assert(not actor:GetSettledDown() and actor:GetSettleKey()==0)
        local mesh=actor.Indicator;local scale=mesh:K2_GetComponentScale()
        local lo,hi={},{};mesh:GetLocalBounds(lo,hi)
        local radius=math.max(math.abs(lo.X),math.abs(hi.X))*math.abs(scale.X)
        if h>=0 and h<470 then assert(math.abs(radius-h)<.01)
        else assert(radius>470 and radius<490) end -- Pinned fountain's native ring.
        assert(mesh:IsVisible())
        observed[h]=(observed[h] or 0)+1;count=count+1
        FCDev.log(string.format('Ghost %x cap %.0f radius %.3f',actor:GetAddress(),h,radius))
    end
end
assert(count>0,'No active fountain ghosts')
if FCPreviewExpected then
    for _,h in ipairs(FCPreviewExpected) do assert(observed[h] and observed[h]>0);observed[h]=observed[h]-1 end
    assert(count==#FCPreviewExpected)
end
assert(FountainControl.settings['0']==nil and not select(1,FCGetNativeLimits(0)))
FCDev.log('PASS: active ghost rings match placement settings without save registration')
