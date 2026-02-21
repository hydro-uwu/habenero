#pragma once
#include <raylib.h>

// ── Engine convenience API — one include to get everything ───────────────────
// Each sub-header mirrors its Lua counterpart exactly so the same logic can be
// written identically in C++ or Lua.
#include <Input/Input.hpp>          // Hotones::Input::   (keyboard + mouse)
#include <Audio/Audio.hpp>          // Hotones::Audio::   (sound bus)
#include <Render/Render.hpp>        // Hotones::Render::  (2-D draw)
#include <Lighting/Lighting.hpp>    // Hotones::Lighting::(dynamic lights)
#include <Draw3D/Draw3D.hpp>        // Hotones::Draw3D::  (3-D primitives)
#include <Physics/PhysicsHelpers.hpp> // Hotones::Physics:: (raycast/sweep result structs)

namespace Hotones {

class core {
public:
  static Camera3D *camera;
  core() {
    camera = new Camera3D();
    camera->position = (Vector3){0.0f, 10.0f, 10.0f};
    camera->target = (Vector3){0.0f, 0.0f, 0.0f};
    camera->up = (Vector3){0.0f, 1.0f, 0.0f};
    camera->fovy = 45.0f;
    camera->projection = CAMERA_PERSPECTIVE;
  }

  ~core() {
    // Destructor code here
    delete camera;
  }

  void Update() {
    // Update game logic here
  }

  void Draw() {
    // Drawing code here
  }
};
} // namespace Hotones