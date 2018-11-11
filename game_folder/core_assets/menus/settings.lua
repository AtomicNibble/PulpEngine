

local function LoadAssets()


end

local function Update()

    ui.fill(2,2,2,220)
    ui.center();

    ui.label("SETTINGS", Color(255,20,20));
    ui.sliderVar("VOLUME MASTER", "snd_vol_master");
    ui.sliderVar("VOLUME MUSIC", "snd_vol_music");
    ui.sliderVar("VOLUME SFX", "snd_vol_sfx");
    ui.sliderVar("VOLUME VOICE", "snd_vol_voice");

    ui.sliderVar("FIELD OF VIEW", "cam_fov", 1);

    ui.checkboxVar("DRAW DEPTH", "r_draw_depth");

    if ui.button("BACK") then
        ui.back()
    end

end


return {
  LoadAssets = LoadAssets,
  Update = Update,
}