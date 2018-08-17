


local function Update(dt)

    ui.fill(0,0,0,255)

    ui.center();
    ui.text("Loading", Color(250,250,250));

    ui.pacifier(dt);

end


return {
  Update = Update,
}