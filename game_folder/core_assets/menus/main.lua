

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
    ui.text("Insert fancy main menu here", Color(255,255 * t,255 * t));
    --ui.popFont();

    if ui.button("SOLO") then
        core.exec("map test01");
        ui.close();
    end
    if ui.button("ONLINE") then
        core.log("nope!")
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