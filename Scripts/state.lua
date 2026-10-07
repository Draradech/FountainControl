return function(root)
    local FC = {settings={}, actors={}, root=root}
    _G.FountainControl=FC
    local function key(actor)
        return tostring(actor:GetSettleKey() & 0xffffffff)
    end
    function FC.is_fountain(actor)
        return actor and actor:IsValid() and actor:GetClass():GetFName():ToString() == 'BP_SmartCannon_C'
            and actor.ThrowIndicator.MovePattern == 2 and actor:GetSettledDown()
    end
    function FC.get(actor)
        local value=FC.settings[key(actor)]
        return value or {drop=1000,horizontal=-1}
    end
    function FC.set(actor,drop,horizontal)
        if not FC.is_fountain(actor) then return end
        drop=math.max(0,math.min(1000,math.floor(drop/50+.5)*50))
        if horizontal<0 or horizontal>500 then horizontal=-1
        else horizontal=math.max(0,math.floor(horizontal/50+.5)*50) end
        FC.settings[key(actor)]={drop=drop,horizontal=horizontal}
        FCSetFacility(actor.SmartCannonFacility:GetAddress(),drop,horizontal)
    end
    function FC.scan()
        local seen={}
        for _,actor in ipairs(FindAllOf('BP_SmartCannon_C') or {}) do
            if FC.is_fountain(actor) then
                local address=actor.SmartCannonFacility:GetAddress()
                seen[address]=actor
                local value=FC.get(actor)
                FCSetFacility(address,value.drop,value.horizontal)
            end
        end
        for address in pairs(FC.actors) do if not seen[address] then FCRemoveFacility(address) end end
        FC.actors=seen
    end
    LoopAsync(1000,function() ExecuteInGameThread(FC.scan,EGameThreadMethod.EngineTick) end)
    return FC
end
