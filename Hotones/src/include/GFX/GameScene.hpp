#pragma once

#include <GFX/Scene.hpp>
#include <GFX/Player.hpp>
#include <raylib.h>
#include <GFX/CollidableModel.hpp>

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
    // Main world model
    CollidableModel* worldModel = nullptr;
    bool worldDebug = false;

    void DrawLevel();
};

} // namespace Hotones
