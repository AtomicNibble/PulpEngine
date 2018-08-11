

local function LoadAssets()


end

local function Update()

    ui.fill(2,2,2,220)
    ui.center();

    if ui.button("RESUME") then
        ui.close()
    end
    if ui.button("SETTINGS") then
        ui.open("settings")
    end
    if ui.button("QUIT") then
        core.exec("quit")
    end

end


return {
  LoadAssets = LoadAssets,
  Update = Update,
}