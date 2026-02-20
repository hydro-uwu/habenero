-- DemoCupProject/init.lua
-- Reference / demo pack for the HoTones engine.
-- Run with:  HoTones --pak DemoCupProject/
-- See docs/cupformat.md for the full spec.

------------------------------------------------------------------------
-- Game state
------------------------------------------------------------------------

Demo = {}
Demo.time    = 0           -- seconds elapsed
Demo.boxRot  = 0           -- box rotation angle (degrees)
Demo.players = {}          -- id -> name table for connected players

------------------------------------------------------------------------
-- Init table (required)
------------------------------------------------------------------------

Init = {
    -- Uncomment to load a model as the world collision mesh:
    -- MainScene = "models/MainScene.gltf",

    -- If MainScene is nil/absent the engine draws a fallback floor + grid.

    MainClass = Demo,
    Debug     = true,
}

------------------------------------------------------------------------
-- Init  called once after the pack loads
------------------------------------------------------------------------

function Demo:Init()
    server.log("DemoCupProject loaded at t=" .. tostring(server.getTime()))
    -- audio.loadSound("hit", "sounds/hit.wav")
    -- audio.setVolume(75)
end

------------------------------------------------------------------------
-- Update  called every tick (~100 Hz server, once per frame client)
------------------------------------------------------------------------

function Demo:Update()
    self.time   = server.getTime()
    self.boxRot = (self.boxRot + 45 * GetFrameTime()) % 360

    if input.isKeyPressed(input.KEY_R) then
        self.boxRot = 0
        server.log("Rotation reset")
    end
    
end

------------------------------------------------------------------------
-- draw3D  called INSIDE BeginMode3D / EndMode3D
--         Use mesh.* here.  render.* will not work correctly here.
------------------------------------------------------------------------

function Demo:draw3D()
    -- Axis marker at origin
    mesh.axes(0, 0.01, 0, 2)

    -- -- Checkerboard floor
    -- local tileSize = 4
    -- local range    = 4
    -- for tx = -range, range do
    --     for tz = -range, range do
    --         local px = tx * tileSize
    --         local pz = tz * tileSize
    --         if (tx + tz) % 2 == 0 then
    --             mesh.plane(px, 0, pz, tileSize, tileSize,  60,  60,  60)
    --         else
    --             mesh.plane(px, 0, pz, tileSize, tileSize,  90,  90,  90)
    --         end
    --     end
    -- end

    -- Spinning box orbiting the pillar
    local bx = math.sin(math.rad(self.boxRot)) * 3
    local bz = math.cos(math.rad(self.boxRot)) * 3
    mesh.box(bx, 1, bz,  1, 1, 1,  255, 120, 40)
    mesh.boxWires(bx, 1, bz,  1.01, 1.01, 1.01,  255, 220, 100)
    -- Cyan pillar at centre
    mesh.cylinder(0, 0, 0,  0.3, 0.3, 4,  12,  80, 200, 220)

    -- Coloured spheres orbiting more slowly
    for i = 1, 6 do
        local a  = math.rad(i * 60 + self.boxRot * 0.5)
        local sx = math.cos(a) * 4
        local sz = math.sin(a) * 4
        local r  = math.floor(100 + i * 25)
        local g  = math.floor(255 - i * 30)
        mesh.sphere(sx, 0.5, sz,  0.4,  8, 8,  r, g, 120)
    end

    -- Nametags for remote players (uses the new network.* API)
    -- Draw a thin plate above each remote player's head so packs can
    -- visually identify players in the 3D world.  This is deliberately
    -- simple and uses mesh primitives so it works inside draw3D().
    for _, p in ipairs(network.getPlayers()) do
        -- small plate above the head
        mesh.box(p.x, p.y + 2.6, p.z,  0.02, 0.28, 0.9,  20, 20, 20)
        -- slight highlight on the front face
        mesh.box(p.x, p.y + 2.6, p.z + 0.47,  0.01, 0.26, 0.6,  220, 220, 220)
    end
end

------------------------------------------------------------------------
-- Draw  called AFTER EndMode3D  (2D / HUD overlay)
--       Use render.* here.  mesh.* will not work correctly here.
------------------------------------------------------------------------

function Demo:Draw()
    render.drawRect(0, 0, 400, 90, 0, 0, 0, 160)
    render.drawText("DemoCupProject", 10, 8,  26, 255, 200, 50)
    render.drawText(
        string.format("t = %.1f s   box = %.0f deg", self.time, self.boxRot),
        10, 42, 16, 180, 180, 180)

    -- Online player list
    local py = 68
    for id, name in pairs(self.players) do
        render.drawText(
            string.format("  [%d] %s", id, name),
            10, py, 14, 100, 220, 120)
        py = py + 18
    end

    -- Controls hint
    render.drawText("R = reset rotation   F1 = debug", 6, 720 - 20, 13, 140, 140, 140)
end

------------------------------------------------------------------------
-- Player events  server-side callbacks
------------------------------------------------------------------------

function Demo:onPlayerJoined(id, name)
    server.log(string.format("++ Player  id=%-3d  name=%s", id, name))
    self.players[id] = name
end

function Demo:onPlayerLeft(id)
    server.log(string.format("-- Player  id=%-3d  left", id))
    self.players[id] = nil
end
