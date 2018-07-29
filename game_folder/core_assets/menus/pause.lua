

local function LoadAssets()


end

local function Update()

    ui.fill(20,20,20,220)

    if ui.button("RESUME") then
        ui.close()
    end
    if ui.button("SETTINGS") then
        ui.open("settings")
    end
    if ui.button("QUIT") then
        core.exec("quit")
    end

    ui.sliderVar("VOLUME", "snd_vol_master");

end


return {
  LoadAssets = LoadAssets,
  Update = Update,
}