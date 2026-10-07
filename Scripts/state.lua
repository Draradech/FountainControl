return function(root)
    local FC = {settings={}, actors={}, indicators={}, root=root}
    local json=dofile(root..'/Scripts/vendor/json.lua')
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
        if not value then
            local found,drop,horizontal=FCGetNativeLimits(actor:GetSettleKey())
            if found then value={drop=drop,horizontal=horizontal}; FC.settings[key(actor)]=value end
        end
        return value or {drop=1000,horizontal=-1}
    end
    function FC.set(actor,drop,horizontal)
        if not FC.is_fountain(actor) then return end
        drop=math.max(0,math.min(1000,math.floor(drop/50+.5)*50))
        if horizontal<0 or horizontal>500 then horizontal=-1
        else horizontal=math.max(0,math.floor(horizontal/50+.5)*50) end
        FC.settings[key(actor)]={drop=drop,horizontal=horizontal}
        FCSetFacility(actor.SmartCannonFacility:GetAddress(),drop,horizontal,actor:GetSettleKey(),actor:GetAddress())
        FC.indicator(actor)
    end
    function FC.indicator(actor)
        local mesh=actor.Indicator
        if not mesh or not mesh:IsValid() then return end
        local address=mesh:GetAddress()
        local baseline=FC.indicators[address]
        if not baseline or baseline.key~=key(actor) then
            local scale=mesh:K2_GetComponentScale()
            local lo,hi={},{}; mesh:GetLocalBounds(lo,hi)
            baseline={key=key(actor),x=scale.X,y=scale.Y,z=scale.Z,r=math.max(math.abs(lo.X),math.abs(hi.X))*math.abs(scale.X)}
            if baseline.r<=0 then return end
            FC.indicators[address]=baseline
        end
        local h=FC.get(actor).horizontal
        local factor=h>=0 and math.min(1,h/baseline.r) or 1
        mesh:SetWorldScale3D({X=baseline.x*factor,Y=baseline.y*factor,Z=baseline.z})
    end
    function FC.load_settings(subsystem)
        local world=subsystem.WorldTitleName:ToString()
        local slot=subsystem.ReloadedName:ToString()
        FC.settings={}; FCResetFacilities()
        if world=='' or slot=='' then return end
        local save=subsystem:GetGameSlotFilePath(world,slot,0):ToString()
        local companion=save:gsub('%.sav$','.fountains.json')
        local file=io.open(companion,'rb')
        if not file then return end
        local text=file:read(1048577); file:close()
        if #text>1048576 then print('[FountainControl] Companion too large; using defaults\n'); return end
        local ok,data=pcall(json.decode,text)
        if not ok or type(data)~='table' or data.schema~=1 or data.fingerprint~=FCFingerprint(save) or type(data.fountains)~='table' then
            print('[FountainControl] Missing, invalid or stale companion; using defaults\n'); return
        end
        for id,value in pairs(data.fountains) do
            if tostring(id):match('^%d+$') and tonumber(id)<=0xffffffff and type(value)=='table' and type(value.drop)=='number' and value.drop>=0 and value.drop<=1000 and value.drop%50==0 then
                local h=value.horizontal
                if h==nil then h=-1 end
                if type(h)=='number' and (h==-1 or h>=0 and h<=500 and h%50==0) then FC.settings[tostring(id)]={drop=value.drop,horizontal=h} end
            end
        end
        print('[FountainControl] Loaded matching save companion\n')
    end
    function FC.scan()
        local subsystem=FindFirstOf('GameSaveLoadSubsystem')
        if subsystem and subsystem:IsValid() then
            FCSetSaveRoot(subsystem:GetGameSlotDirectory():ToString())
            local game=subsystem.GameState
            local address=game and game:IsValid() and game:GetAddress() or 0
            local generation=FCLoadGeneration()
            if address~=FC.game or generation~=FC.generation then
                FC.game=address; FC.generation=generation; FC.indicators={}
                FC.load_settings(subsystem)
            end
        end
        local seen,meshes={},{}
        for _,actor in ipairs(FindAllOf('BP_SmartCannon_C') or {}) do
            if FC.is_fountain(actor) then
                local address=actor.SmartCannonFacility:GetAddress()
                seen[address]=actor
                local value=FC.get(actor)
                FCSetFacility(address,value.drop,value.horizontal,actor:GetSettleKey(),actor:GetAddress())
                FC.indicator(actor)
                if actor.Indicator and actor.Indicator:IsValid() then meshes[actor.Indicator:GetAddress()]=true end
            end
        end
        for address in pairs(FC.actors) do if not seen[address] then FCRemoveFacility(address) end end
        for address in pairs(FC.indicators) do if not meshes[address] then FC.indicators[address]=nil end end
        FC.actors=seen
    end
    LoopInGameThreadWithDelay(1000,FC.scan)
    return FC
end
