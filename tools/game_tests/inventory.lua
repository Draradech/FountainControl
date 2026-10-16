-- Read-only inventory for selecting objects in a fresh disposable test world.
local s=FindFirstOf('GameSaveLoadSubsystem')
assert(s and s:IsValid())
FCDev.log('World: '..s.WorldTitleName:ToString()..'; slot: '..s.ReloadedName:ToString())
for _,a in ipairs(FindAllOf('BP_SmartCannon_C') or {}) do
    if a:IsValid() and a:GetSettledDown() then
        local p=a:K2_GetActorLocation()
        FCDev.log(string.format('Fountain key=%u address=%x position=(%.1f,%.1f,%.1f) getter=%.6f',
            a:GetSettleKey(),a:GetAddress(),p.X,p.Y,p.Z,a.ThrowIndicator:GetHorizontalRadius()))
    end
end
for _,c in ipairs(FindAllOf('HopperCatcherComponent') or {}) do
    if c:IsValid() and c:GetOwner():GetSettledDown() then FCDev.log('Receiver: '..c:GetFullName()) end
end
