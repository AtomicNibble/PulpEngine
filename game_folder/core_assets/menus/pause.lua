

local function LoadAssets()


end

local totalTime = 0;

local function fadeIn(dt)

    totalTime = totalTime + dt;

    local speed = 900;
    local t = math.min(1, totalTime / speed);

    ui.fill(2,2,2,220 * t)
end

local function Update(dt)

    fadeIn(dt);

    ui.center();

    if ui.button("RESUME") then
        ui.close()
    end
    if ui.button("SETTINGS") then
        ui.open("settings")
    end
    if ui.button("QUIT") then
        core.exec("mainMenu")
        ui.close();
    end

end

local function OnOpen()
    totalTime = 0;
end

return {
  LoadAssets = LoadAssets,
  Update = Update,
  OnOpen = OnOpen,
}