-- Read-only component-template/default query, safe to send through the mailbox.
-- If templates are not loaded, open the build menu/place the relevant buildings.
for _,name in ipairs({'BP_SmartCannon','BP_SmartCannonCeiling'}) do
    for _,component in ipairs({'ThrowIndicator','SmartCannonFacility'}) do
        local path='/Game/Blueprints/Buildings/'..name..'.'..name..'_C:'..component..'_GEN_VARIABLE'
        local obj=FindObject(nil,nil,path,false)
        if not obj or not obj:IsValid() then
            FCDev.log('NOT LOADED: '..path)
        else
            FCDev.log(obj:GetFullName())
            local fields=component=='ThrowIndicator'
                and {'MovePattern','MinVerticalSpeed','MaxVerticalSpeed','HorizontalSpeed'}
                or {'SmartCannonMaxDistance','HorizontalSpeed','SmartCannonSpeed','UseDrop'}
            for _,key in ipairs(fields) do FCDev.log(key..'='..tostring(obj[key])) end
            if component=='ThrowIndicator' then
                FCDev.log('GetHorizontalRadius='..tostring(obj:GetHorizontalRadius()))
            else
                local p=obj.EmitPosition
                FCDev.log(string.format('EmitPosition=(%s,%s,%s)',p.X,p.Y,p.Z))
            end
        end
    end
end
