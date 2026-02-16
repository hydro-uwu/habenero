#pragma once

#include "../include/Scene.hpp"
#include "../include/Player.hpp"
#include <raylib.h>

namespace Hotones {

class GameScene : public Scene {
public:
    GameScene();
    ~GameScene() override = default;

    void Init() override;
    void Update() override;
    void Draw() override;
    void Unload() override;

private:
    Hotones::Player player;
    Camera camera;

    void DrawLevel();
};

} // namespace Hotones
