

local function LoadAssets()


end


local function Update(dt)

    local t = ((math.sin(uiTimeMS / 1000) + 1.0) * 0.5)

    ui.fill(2,2,2,220);
    ui.center();

    --really I want to just have normal and big font sizes.
    --ui.pushFont();
    --ui.setTextSize(36,36);
    --ui.setTextFlags();
    --ui.popFont();
    ui.label("MAIN", Color(255,20,20));

    if ui.button("SOLO test01") then
        core.exec("map test01");
    end
    if ui.button("SOLO text_cord_test") then
        core.exec("map text_cord_test");
    end

    if ui.button("ONLINE") then
        ui.open("online")
    end
    if ui.button("SETTINGS") then
        ui.open("settings")
    end
    if ui.button("QUIT") then
        core.exec("quit")
    end

end

local function OnOpen()

end


return {
  LoadAssets = LoadAssets,
  Update = Update,
  OnOpen = OnOpen,
}