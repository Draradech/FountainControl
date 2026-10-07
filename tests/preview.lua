-- Ghosts share a zero building ID and can be pooled between placements.
-- Check visual independence, cleanup and reuse without registering facilities.
local root=assert(arg[1])
LoopInGameThreadWithDelay=function()end
FCGetNativeLimits=function()error('Ghosts must not read settings by SettleKey')end
FCSetFacility=function()error('Ghosts must not enter live facility/save maps')end
local pc={IsValid=function()return true end,GetAddress=function()return 42 end}
local actors,limits={},{}
FindFirstOf=function()return pc end
FindAllOf=function()return actors end
FCGetGhostPreviews=function(address)assert(address==42);return limits end
local FC=dofile(root..'/Scripts/state.lua')(root)
local function ghost(address)
 local scale={X=10,Y=10,Z=1}
 local mesh={IsValid=function()return true end,GetAddress=function()return address+100 end,
 K2_GetComponentScale=function()return scale end,
 GetLocalBounds=function(_,lo,hi)lo.X=-50;hi.X=50 end,
 SetWorldScale3D=function(_,value)scale=value end}
 local actor={Indicator=mesh,IsValid=function()return true end,GetAddress=function()return address end,
 GetSettleKey=function()return 0 end,GetSettledDown=function()return false end,
 ShowIndicator=function(self,value)self.shown=value end}
 return actor,function()return scale end
end
local a,scaleA=ghost(1);local b,scaleB=ghost(2);actors={a,b};limits={[1]=150,[2]=300}
FC.preview();assert(a.shown and b.shown);assert(scaleA().X==3 and scaleB().X==6)
FC.preview();assert(scaleA().X==3 and scaleB().X==6,'Repeated polling must not compound shrinkage')
limits={[1]=0,[2]=-1};FC.preview();assert(scaleA().X==0 and scaleB().X==10)
limits={};FC.preview();assert(scaleA().X==10 and scaleB().X==10 and not a.shown and not b.shown)
assert(next(FC.preview_indicators)==nil and next(FC.settings)==nil and next(FC.indicators)==nil)
limits={[1]=-1};FC.preview();assert(scaleA().X==10 and a.shown,'Reused ghost must regain original Default radius')
assert(next(FC.settings)==nil and next(FC.indicators)==nil)
print('Ghost previews: separate zero-ID actors, zero/default, repeated polling, cancellation and reuse passed')
