#pragma once
#include <raylib.h>
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