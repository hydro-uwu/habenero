#pragma once

#include "../include/Scene.hpp"
#include <raylib.h>

namespace Hotones {

class SimpleScene : public Scene {
public:
    SimpleScene(const char* label = "Next Scene", float durationSeconds = 2.0f);
    void Init() override;
    void Update() override;
    void Draw() override;
private:
    const char* text;
    float elapsed;
    float duration;
};

} // namespace Hotones
