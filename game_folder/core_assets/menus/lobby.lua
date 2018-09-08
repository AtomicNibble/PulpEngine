

local function LoadAssets()

end

local function Update(dt)

    local t = ((math.sin(uiTimeMS / 1000) + 1.0) * 0.5)

    ui.fill(2,2,2,220);
    ui.center();

    if ui.button("START") then
        core.exec("map test01");
        ui.close();
    end
    if ui.button("BACK") then
        ui.back()
    end

end

local function OnOpen()

end

return {
  LoadAssets = LoadAssets,
  Update = Update,
  OnOpen = OnOpen,
}