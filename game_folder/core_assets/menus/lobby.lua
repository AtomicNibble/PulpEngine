

local function LoadAssets()

end

local function IsHost()
  return core.getDvarInt("net_host") ~= 0;
end

local function Update(dt)

    local t = ((math.sin(uiTimeMS / 1000) + 1.0) * 0.5)

    ui.fill(2,2,2,220);
    ui.center();

    ui.list();
    ui.label("LOBBY", Color(255,20,20));

    if ui.button("START GAME") then
        core.exec("startMatch");
    end
    if ui.button("BACK") then
        core.exec("mainMenu");
    end

end

local function OnOpen()

end

return {
  LoadAssets = LoadAssets,
  Update = Update,
  OnOpen = OnOpen,
}