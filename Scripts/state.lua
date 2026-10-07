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
    function FC.indicator(actor,horizontal,cache)
        local mesh=actor.Indicator
        if not mesh or not mesh:IsValid() then return end
        local address=mesh:GetAddress()
        cache=cache or FC.indicators
        local baseline=cache[address]
        if not baseline or baseline.key~=key(actor) or baseline.owner~=actor:GetAddress() then
            local scale=mesh:K2_GetComponentScale()
            local lo,hi={},{}; mesh:GetLocalBounds(lo,hi)
            baseline={key=key(actor),owner=actor:GetAddress(),mesh=mesh,actor=actor,x=scale.X,y=scale.Y,z=scale.Z,r=math.max(math.abs(lo.X),math.abs(hi.X))*math.abs(scale.X)}
            if baseline.r<=0 then return end
            cache[address]=baseline
        end
        local h=horizontal
        if h==nil then h=FC.get(actor).horizontal end
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
        if not ok or type(data)~='table' or data.schema~=1 or type(data.fingerprint)~='string' or type(data.fountains)~='table' then
            print('[FountainControl] Invalid save companion; using defaults\n'); return
        end
        local fingerprint=FCFingerprint(save)
        if fingerprint=='' or data.fingerprint~=fingerprint then
            local reason=fingerprint=='' and 'Save checksum unavailable' or 'Save checksum mismatch'
            local warning='[FountainControl] WARNING: '..reason..' for '..world..'/'..slot
                ..'; restoring valid fountain settings by persistent SettleKey. Settings may be stale.'
            FCLog(warning)
            print(warning..'\n')
        end
        for id,value in pairs(data.fountains) do
            if tostring(id):match('^%d+$') and tonumber(id)<=0xffffffff and type(value)=='table' and type(value.drop)=='number' and value.drop>=0 and value.drop<=1000 and value.drop%50==0 then
                local h=value.horizontal
                if h==nil then h=-1 end
                if type(h)=='number' and (h==-1 or h>=0 and h<=500 and h%50==0) then FC.settings[tostring(id)]={drop=value.drop,horizontal=h} end
            end
        end
        print('[FountainControl] Loaded save companion\n')
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
    FC.preview_indicators={}
    function FC.preview()
        local limits={}
        local pc=FindFirstOf('BeltTDPlayerController')
        if pc and pc:IsValid() then limits=FCGetGhostPreviews(pc:GetAddress()) end
        local seen={}
        if next(limits) then
            for _,actor in ipairs(FindAllOf('BP_SmartCannon_C') or {}) do
                local horizontal=limits[actor:GetAddress()]
                if horizontal~=nil and actor:IsValid() and not actor:GetSettledDown() then
                    actor:ShowIndicator(true)
                    FC.indicator(actor,horizontal,FC.preview_indicators)
                    if actor.Indicator and actor.Indicator:IsValid() then seen[actor.Indicator:GetAddress()]=true end
                end
            end
        end
        for address,baseline in pairs(FC.preview_indicators) do
            if not seen[address] then
                -- Restore before a pooled ghost is reused for another selection.
                if baseline.actor:IsValid() and baseline.mesh:IsValid()
                    and baseline.actor.Indicator:GetAddress()==address and not baseline.actor:GetSettledDown() then
                    baseline.mesh:SetWorldScale3D({X=baseline.x,Y=baseline.y,Z=baseline.z})
                    baseline.actor:ShowIndicator(false)
                end
                FC.preview_indicators[address]=nil
            end
        end
    end
    LoopInGameThreadWithDelay(100,FC.preview)
    LoopInGameThreadWithDelay(1000,FC.scan)
    return FC
end
