

local function LoadAssets()

end

local function Update(dt)

    local t = ((math.sin(uiTimeMS / 1000) + 1.0) * 0.5)

    ui.fill(2,2,2,220);
    ui.center();
    ui.label("ONLINE", Color(255,20,20));

    if ui.button("CREATE LOBBY") then
        core.exec("createLobby");
    end
    if ui.button("JOIN LOBBY") then
        -- core.exec("map text_cord_test");
        core.log("connect manually")
    end

    if ui.button("BACK") then
        ui.back();
    end

end

local function OnOpen()

end

return {
  LoadAssets = LoadAssets,
  Update = Update,
  OnOpen = OnOpen,
}