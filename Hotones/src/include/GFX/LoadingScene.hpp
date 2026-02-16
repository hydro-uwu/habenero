#pragma once

#include <GFX/Scene.hpp>
#include <raylib.h>

#define LOADING_STAR_COUNT 420

namespace Hotones {

class LoadingScene : public Scene {
public:
    LoadingScene(float durationSeconds = 3.0f);
    virtual ~LoadingScene() override = default;

    void Init() override;
    void Update() override;
    void Draw() override;
    void Unload() override;

private:
    Vector3 stars[LOADING_STAR_COUNT];
    Vector2 starsScreenPos[LOADING_STAR_COUNT];
    Color bgColor;
    float speed;
    bool drawLines;
    float elapsed;
    float duration;
};

} // namespace Hotones
