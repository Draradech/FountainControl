-- Run through tools/probe.py in a disposable FountainControl_*Dev world.
-- Set FCDeliveryFountainKey to its settled fountain's key. Optionally select a
-- particular receiver with FCDeliveryReceiverName (GetFullName()).
local s=FindFirstOf('GameSaveLoadSubsystem')
assert((s.WorldTitleName:ToString():match('^FountainControl_.*Dev$') or
    s.WorldTitleName:ToString():match('^FountainControl_.*Dev_%d%d%d%d%d%d%d%d%d%d$')),'Test world required')
local FC=FountainControl
FC.scan()
local key=assert(FCDeliveryFountainKey,'Set FCDeliveryFountainKey from a fresh world inventory')
local actor
for _,a in pairs(FC.actors) do if a:GetSettleKey()==key then actor=a end end
assert(actor,'Test fountain missing')
local receiver
for _,c in ipairs(FindAllOf('HopperCatcherComponent') or {}) do
 if c:IsValid() and c:GetOwner():GetSettledDown() and (not FCDeliveryReceiverName or c:GetFullName()==FCDeliveryReceiverName) then receiver=c; break end
end
assert(receiver)
local owner=receiver:GetOwner()
local original=owner:K2_GetActorLocation()
local position={X=original.X,Y=original.Y,Z=original.Z}
local settings=FC.get(actor)
local old={drop=settings.drop,horizontal=settings.horizontal}
local facility=actor.SmartCannonFacility:GetAddress()
local _,_,ox,oy,oz=FCProbeReach(facility,0,0,0)
local cx,cy,cz=FCProbeCatch(receiver:GetAddress())
local offset={X=cx-position.X,Y=cy-position.Y,Z=cz-position.Z}
local visuals=FindFirstOf('ProductVisualSystem')
local item=FName('SilverCoin'):GetComparisonIndex()
local cases={
 {'vertical down',0,-100,1000,-1,true},
 {'vertical level',0,0,1000,-1,true},
 {'vertical up',0,100,1000,-1,true},
 {'inner level',50,0,1000,-1,true},
 {'drop boundary',0,-100,100,250,true},
 {'beyond drop',0,-100.01,100,250,false},
 {'zero drop',0,-1,0,-1,false},
 {'horizontal boundary',250,0,1000,250,true},
 {'beyond horizontal',250.01,0,1000,250,false},
 {'numbered maximum',500,0,1000,500,true},
 {'default farther below',800,-500,1000,-1,true},
 {'zero radius',0,-100,1000,0,true},
 {'original drop maximum',0,-1000,1000,-1,true},
 {'beyond original drop',0,-1000.01,1000,-1,false},
}
local function count()
 local total=0
 receiver.HooperComp.CachedItemsNew:ForEach(function(_,v)
  local a=v:get(); if a.IngredientName:ToString()=='SilverCoin' then total=total+a.Count end
 end)
 return total
end
local index=0
local waiting,expected,deadline,before
local completed=false
LoopInGameThreadWithDelay(100,function()
 if completed then return true end
 local ok,err=xpcall(function()
  if waiting then
   if os.clock()<deadline then return end
   assert(receiver.HooperComp.ThrowItemsNew:GetArrayNum()==0,'Flight did not finish')
   assert(count()==before+(expected and 1 or 0),'Inventory arrival mismatch: '..cases[index][1])
   FCDev.log('PASS delivery: '..cases[index][1]); waiting=false
  end
  index=index+1
  local c=cases[index]
  if not c then
   owner:K2_SetActorLocation(position,false,{},true)
   FC.set(actor,old.drop,old.horizontal)
   FCDev.log('DELIVERY COMPLETE: '..#cases..' cases; repaired counters '..table.concat({FCFlightStats()},','))
   completed=true; return
  end
  FC.set(actor,c[4],c[5]); expected=c[6]
  owner:K2_SetActorLocation({X=ox+c[2]-offset.X,Y=oy-offset.Y,Z=oz+c[3]-offset.Z},false,{},true)
  before=count()
  local _,modified,_,_,_,time=FCProbeReach(facility,c[2],0,c[3])
  assert(modified==expected,'Reach mismatch: '..c[1])
  local delivered=FCProbeDeliver(facility,receiver:GetAddress(),s:GetAddress(),visuals:GetAddress(),item)
  assert(delivered==expected,'Enqueue mismatch: '..c[1])
  waiting=true; deadline=os.clock()+(expected and time or 0)+.3
 end,debug.traceback)
 if not ok then
  FCDev.log('DELIVERY FAILED: '..err); completed=true
  owner:K2_SetActorLocation(position,false,{},true); FC.set(actor,old.drop,old.horizontal)
 end
 return completed
end)
FCDev.log('Delivery matrix started; watch probe.log for DELIVERY COMPLETE')
