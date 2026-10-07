-- Extend the game's storage panel rather than creating a second interaction screen.
return function(FC)
    local panels={}
    local function construct(panel)
        local tree=panel.WidgetTree
        local function widget(class,name)
            return StaticConstructObject(StaticFindObject('/Script/UMG.'..class),tree,FName(name))
        end
        local box=widget('VerticalBox','FountainControlControls')
        local slot=tree.RootWidget:AddChildToVerticalBox(box)
        slot:SetPadding({Left=8,Top=12,Right=8,Bottom=8})
        local controls={panel=panel,box=box}
        local function row(name,max)
            local label=widget('TextBlock','FountainControl'..name..'Label')
            local font=panel.Txt_Title.Font; font.Size=18
            label:SetFont(font)
            label:SetColorAndOpacity({SpecifiedColor={R=.08,G=.14,B=.19,A=1},ColorUseRule=0})
            box:AddChildToVerticalBox(label):SetPadding({Left=0,Top=4,Right=0,Bottom=2})
            local slider=widget('Slider','FountainControl'..name..'Slider')
            slider:SetMinValue(0); slider:SetMaxValue(max); slider:SetStepSize(1)
            slider.MouseUsesStep=true
            box:AddChildToVerticalBox(slider):SetPadding({Left=0,Top=0,Right=0,Bottom=8})
            return {label=label,slider=slider}
        end
        controls.drop=row('Drop',20); controls.horizontal=row('Horizontal',11)
        return controls
    end
    local function update(c)
        local actor=c.panel.ContextActor
        if not c.panel:IsActivated() or not FC.is_fountain(actor) then c.box:SetVisibility(1); c.actor=nil; return end
        c.box:SetVisibility(0)
        local value=FC.get(actor)
        if c.actor~=actor:GetAddress() then
            c.actor=actor:GetAddress()
            c.drop.slider:SetValue(value.drop/50)
            c.horizontal.slider:SetValue(value.horizontal<0 and 11 or value.horizontal/50)
        else
            local drop=math.floor(c.drop.slider:GetValue()+.5)*50
            local position=math.floor(c.horizontal.slider:GetValue()+.5)
            local horizontal=position==11 and -1 or position*50
            if drop~=value.drop or horizontal~=value.horizontal then FC.set(actor,drop,horizontal); value=FC.get(actor) end
        end
        c.drop.slider:SetValue(value.drop/50)
        c.horizontal.slider:SetValue(value.horizontal<0 and 11 or value.horizontal/50)
        c.drop.label:SetText(FText(string.format('Drop limit: %.1f m',value.drop/100)))
        c.horizontal.label:SetText(FText(value.horizontal<0 and 'Horizontal limit: Default' or string.format('Horizontal limit: %.1f m',value.horizontal/100)))
        FC.indicator(actor)
    end
    local function tick()
        for address,c in pairs(panels) do if not c.panel:IsValid() then panels[address]=nil end end
        for _,panel in ipairs(FindAllOf('WBP_StoragePanel_C') or {}) do
            local address=panel:GetAddress()
            if panel:IsActivated() and FC.is_fountain(panel.ContextActor) and not panels[address] then panels[address]=construct(panel) end
        end
        for _,c in pairs(panels) do update(c) end
    end
    LoopAsync(100,function() ExecuteInGameThread(tick,EGameThreadMethod.EngineTick) end)
    FC.panels=panels
end
