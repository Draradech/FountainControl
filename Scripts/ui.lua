-- Use the normal Inspect input and CommonUI interaction layer. The storage
-- widget supplies the game's context/lifecycle; fountains get their own body.
return function(FC)
    local panels,interactions={},{}
    local function templates()
        local bank=StaticFindObject('/Game/Blueprints/WBP/FurnitureInfo/WBP_BankPanel.WBP_BankPanel_C:WidgetTree.Sld_SupplyLimit')
        if not bank or not bank:IsValid() then return end
        return bank,bank:GetParent():GetParent():GetParent()
    end
    local function construct(panel,bank,frame)
        local tree=panel.WidgetTree
        local function widget(class,name)
            return StaticConstructObject(StaticFindObject('/Script/UMG.'..class),tree,FName('FountainControl'..name))
        end
        local root=tree.RootWidget
        local controls={panel=panel,original={}}
        for i=0,root:GetChildrenCount()-1 do
            local child=root:GetChildAt(i)
            controls.original[#controls.original+1]={widget=child,visibility=child:GetVisibility()}
        end
        local group=widget('VerticalBox','Controls')
        local size=widget('SizeBox','Size');size:SetWidthOverride(600);size:SetContent(group)
        local slot=root:AddChildToVerticalBox(size);slot:SetHorizontalAlignment(2)
        controls.box=size
        local title=widget('TextBlock','Title');title:SetFont(panel.Txt_Title.Font)
        title:SetColorAndOpacity(panel.Txt_Title.ColorAndOpacity);title:SetJustification(1)
        controls.title=title
        local titleBorder=widget('Border','TitleBackground')
        titleBorder:SetBrush(panel.Txt_Title:GetParent():GetParent().Background)
        titleBorder:SetPadding({Left=12,Top=6,Right=12,Bottom=6});titleBorder:SetContent(title)
        local titleSize=widget('SizeBox','TitleSize');titleSize:SetWidthOverride(400);titleSize:SetHeightOverride(65);titleSize:SetContent(titleBorder)
        group:AddChildToVerticalBox(titleSize):SetHorizontalAlignment(2)
        local box=widget('VerticalBox','Rows')
        local body=widget('Border','Frame');body:SetBrush(frame.Background)
        body:SetBrushColor(frame.BrushColor);body:SetPadding(frame.Padding);body:SetContent(box)
        group:AddChildToVerticalBox(body)
        local function row(name,max)
            local label=widget('TextBlock',name..'Label')
            label:SetFont(panel.Txt_Title.Font)
            local font=label.Font;font.Size=18;label:SetFont(font)
            label:SetColorAndOpacity(panel.Txt_Title.ColorAndOpacity)
            box:AddChildToVerticalBox(label):SetPadding({Left=0,Top=4,Right=0,Bottom=6})
            local slider=widget('Slider',name..'Slider')
            -- Set before the Slate widget is constructed by attachment. Copy
            -- the complete style, including hovered/disabled texture brushes.
            slider.WidgetStyle=bank.WidgetStyle
            slider:SetSliderBarColor(bank.SliderBarColor);slider:SetSliderHandleColor(bank.SliderHandleColor)
            slider:SetIndentHandle(bank.IndentHandle)
            slider:SetMinValue(0);slider:SetMaxValue(max);slider:SetStepSize(1)
            slider.MouseUsesStep=true
            box:AddChildToVerticalBox(slider):SetPadding({Left=0,Top=0,Right=0,Bottom=16})
            return {label=label,slider=slider}
        end
        controls.drop=row('Drop',20);controls.horizontal=row('Horizontal',11)
        return controls
    end
    local function update(c)
        local actor=c.panel.ContextActor
        local fountain=c.panel:IsActivated() and FC.is_fountain(actor)
        c.box:SetVisibility(fountain and 0 or 1)
        if fountain~=c.replaced then
            for _,child in ipairs(c.original) do child.widget:SetVisibility(fountain and 1 or child.visibility) end
            c.replaced=fountain
        end
        if not fountain then c.actor=nil;return end
        local value=FC.get(actor)
        if c.actor~=actor:GetAddress() then
            c.actor=actor:GetAddress()
            c.drop.slider:SetValue(value.drop/50)
            c.horizontal.slider:SetValue(value.horizontal<0 and 11 or value.horizontal/50)
        else
            local drop=math.floor(c.drop.slider:GetValue()+.5)*50
            local position=math.floor(c.horizontal.slider:GetValue()+.5)
            local horizontal=position==11 and -1 or position*50
            if drop~=value.drop or horizontal~=value.horizontal then FC.set(actor,drop,horizontal);value=FC.get(actor) end
        end
        c.drop.slider:SetValue(value.drop/50)
        c.horizontal.slider:SetValue(value.horizontal<0 and 11 or value.horizontal/50)
        c.title:SetText(c.panel.Txt_Title:GetText())
        c.drop.label:SetText(FText(string.format('Drop limit: %.1f m',value.drop/100)))
        c.horizontal.label:SetText(FText(value.horizontal<0 and 'Horizontal limit: Default' or string.format('Horizontal limit: %.1f m',value.horizontal/100)))
        FC.indicator(actor)
    end
    local function register_interactions()
        local seen={}
        for _,actor in pairs(FC.actors) do
            if FC.is_fountain(actor) then
                local address=actor:GetAddress()
                local entry=interactions[address]
                if not entry or not entry.component:IsValid() or entry.key~=actor:GetSettleKey() then
                    local component=actor:AddComponentByClass(StaticFindObject('/Script/BeltTD.ScriptInteractionComponent'),false,
                        {Rotation={X=0,Y=0,Z=0,W=1},Translation={X=0,Y=0,Z=0},Scale3D={X=1,Y=1,Z=1}},false)
                    component:RegisterInteractionInfoList({FName('Inspect')})
                    entry={component=component,key=actor:GetSettleKey()};interactions[address]=entry
                end
                seen[address]=true
                FCSetInteraction(entry.component:GetAddress(),address)
            end
        end
        for address,entry in pairs(interactions) do
            if not seen[address] then FCSetInteraction(entry.component:GetAddress(),0);interactions[address]=nil end
        end
    end
    local function tick()
        local requested=FCTakeInteraction()
        if requested~=0 then
            for _,actor in pairs(FC.actors) do
                if actor:GetAddress()==requested and FC.is_fountain(actor) then
                    local hud=FindFirstOf('BeltTDHUD')
                    if hud and hud:IsValid() then hud:ToggleInteractionWidget(FName('GameMenu'),FName('StorageInspectWidget'),actor) end
                    break
                end
            end
        end
        for address,c in pairs(panels) do if not c.panel:IsValid() then panels[address]=nil end end
        local bank,frame=templates()
        if bank then
            for _,panel in ipairs(FindAllOf('WBP_StoragePanel_C') or {}) do
                local address=panel:GetAddress()
                if panel:IsActivated() and FC.is_fountain(panel.ContextActor) and not panels[address] then panels[address]=construct(panel,bank,frame) end
            end
        end
        for _,c in pairs(panels) do update(c) end
    end
    LoopInGameThreadWithDelay(1000,register_interactions)
    LoopInGameThreadWithDelay(100,tick)
    FC.panels=panels;FC.interactions=interactions
end
