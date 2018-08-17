


local function Update()

    ui.fill(0,0,0,255)

    ui.center();
    ui.text("Loading", Color(250,250,250));

end


return {
  Update = Update,
}