# Cup — Custom Unified Package Format

With **Cup** (`.cup`) you can bundle a complete game or game-mode and hand it
straight to a Hotones dedicated server — similar to how Minetest/Luanti game
packs work.

---

## What is a .cup file?

A `.cup` file is just a **ZIP archive** renamed to `.cup`.  
You can create one with any standard zip tool:

```sh
# Linux / macOS
zip -r mygame.cup init.lua models/ scripts/ sounds/

# Windows (PowerShell)
Compress-Archive -Path init.lua, models, scripts, sounds -DestinationPath mygame.cup
```

---

## Directory layout

```
mygame.cup  (zip)
├── init.lua          ← required entry point
├── models/
│   └── MainScene.gltf
├── scripts/          ← optional extra Lua files
└── sounds/
```

---

## init.lua reference

`init.lua` is executed first. It must declare a global `Init` table and a
class table that implements the game lifecycle callbacks.

```lua
MyGame = {}   -- your game class (used as a singleton)

-- Required: engine reads this table before starting the game loop
Init = {
    -- Path to the main scene, relative to the pack root.
    -- Supports .gltf, .glb, .obj, and anything Raylib can load as a model.
    -- If absent, the engine renders a fallback dark floor + grid.
    MainScene = "models/MainScene.gltf",

    -- Reference to your game class table.
    MainClass = MyGame,

    -- Optional: print extra engine diagnostics
    Debug = false,
}

-- Called once after the pack is loaded (server AND client).
function MyGame:Init()
    print("MyGame started!")
end

-- Called every server tick (~100 Hz, i.e. every 10 ms).
-- Also called each render frame on the client.
function MyGame:Update()
end

-- Called INSIDE BeginMode3D / EndMode3D (client only).
-- Use mesh.* functions here to draw 3D geometry.
function MyGame:draw3D()
    mesh.grid(20, 1)
    mesh.box(0, 0.5, 0,  1, 1, 1,  255, 100, 50)
end

-- Called AFTER EndMode3D (client only).
-- Use render.* functions here for 2D HUD elements drawn on top of the 3D scene.
function MyGame:Draw()
    render.drawText("Hello from MyGame!", 10, 10, 24)
end
```

### Built-in Lua APIs

#### `mesh` — 3D primitive generation

> **Must be called from `draw3D()` only** — the engine calls this callback
> inside `BeginMode3D() / EndMode3D()`.  Calling `mesh.*` from `Draw()` or
> `Update()` has no effect.

| Function | Description |
|----------|-------------|
| `mesh.plane(x,y,z, w,d [,r,g,b,a])` | Flat XZ-axis-aligned plane |
| `mesh.box(x,y,z, w,h,d [,r,g,b,a])` | Solid box |
| `mesh.boxWires(x,y,z, w,h,d [,r,g,b,a])` | Wireframe box |
| `mesh.sphere(x,y,z, radius [,rings,slices, r,g,b,a])` | Sphere |
| `mesh.cylinder(x,y,z, rTop,rBot,h [,slices, r,g,b,a])` | Cylinder / cone |
| `mesh.line(x1,y1,z1, x2,y2,z2 [,r,g,b,a])` | 3D line segment |
| `mesh.grid(slices, spacing)` | Debug XZ grid |
| `mesh.axes(x,y,z, size)` | RGB axis arrows (red=X, green=Y, blue=Z) |

| Function | Description |
|----------|-------------|
| `render.drawText(text, x, y [,size, r, g, b, a])` | Draw text on screen |
| `render.clearScreen([r, g, b, a])` | Clear framebuffer |
| `render.drawRect(x, y, w, h [,r, g, b, a])` | Draw filled rectangle |

#### `audio` — sound playback

| Function | Description |
|----------|-------------|
| `audio.loadSound(name, path)` → `bool` | Load a file under a logical group name |
| `audio.play(name [, gain])` → `bool` | Play first variant of a loaded group |
| `audio.playRandom(name [, gain])` → `bool` | Play a random variant |
| `audio.playSequential(name [, gain])` → `bool` | Round-robin through variants |
| `audio.setVolume(vol)` | Set master volume 0–100 |
| `audio.getVolume()` → `int` | Get current master volume |
| `audio.stopAll()` | Stop all playing sounds |

#### `input` — keyboard & mouse (client only; no-op on headless)

| Function | Description |
|----------|-------------|
| `input.isKeyDown(key)` → `bool` | Key held this frame |
| `input.isKeyPressed(key)` → `bool` | Key first pressed this frame |
| `input.isKeyReleased(key)` → `bool` | Key first released this frame |
| `input.isMouseDown(btn)` → `bool` | Mouse button held (0=left, 1=right, 2=middle) |
| `input.isMousePressed(btn)` → `bool` | Mouse button first pressed this frame |
| `input.getMousePos()` → `x, y` | Screen position |
| `input.getMouseDelta()` → `dx, dy` | Movement since last frame |
| `input.getMouseWheel()` → `float` | Scroll wheel delta |

Key constants on the `input` table: `KEY_A`–`KEY_Z`, `KEY_0`–`KEY_9`,
`KEY_F1`–`KEY_F12`, `KEY_SPACE`, `KEY_ENTER`, `KEY_ESCAPE`, `KEY_TAB`,
`KEY_BACKSPACE`, `KEY_DELETE`, `KEY_UP/DOWN/LEFT/RIGHT`,
`KEY_LSHIFT`, `KEY_RSHIFT`, `KEY_LCTRL`, `KEY_RCTRL`, `KEY_LALT`, `KEY_RALT`,
`MOUSE_LEFT`, `MOUSE_RIGHT`, `MOUSE_MIDDLE`.

#### `server` — server utilities

| Function | Description |
|----------|-------------|
| `server.log(msg)` | Print to stdout with a `[Script]` prefix |
| `server.getTime()` → `float` | Seconds elapsed since the pack was loaded |

#### Player event callbacks (on `MainClass`)

These methods are **optional**. If defined, the engine calls them automatically.

```lua
-- Called when a player connects (server-side).
function MyGame:onPlayerJoined(id, name) end

-- Called when a player disconnects (server-side).
function MyGame:onPlayerLeft(id) end
```

---

## Hosting a game pack

```sh
# Headless dedicated server — loads mygame.cup, listens on default port 27015
Hotones --server --pak path/to/mygame.cup

# Custom port
Hotones --server --port 7777 --pak path/to/mygame.cup

# During development you can point directly at an extracted directory
Hotones --server --pak path/to/DemoCupProject/
```

---

## Other command-line flags

| Flag | Default | Description |
|------|---------|-------------|
| `--server` | — | Run as headless dedicated server |
| `--pak <path>` | — | `.cup` archive or unpacked directory to host |
| `--port <n>` | `27015` | UDP port the server listens on |
| `--connect <host>` | — | Connect to a remote server (client mode) |
| `--cport <n>` | `27015` | Remote port to connect to |
| `--name <str>` | `Player` | Player display name |

---

## Implementation notes

* `.cup` files are extracted to `<system temp>/hotones_cup_<name>/` at startup
  and cleaned up automatically when the server shuts down.
* A plain directory path also works — useful during development without
  needing to repack every change.
* Zip extraction uses [miniz](https://github.com/richgel999/miniz) (vendored,
  public domain, drop `miniz.h` into `src/include/`).
