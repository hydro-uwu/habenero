#pragma once

#include "../include/Scene.hpp"
#include <raylib.h>
#include <vector>
#include <functional>

namespace Hotones {

class TransitionScene : public Scene {
public:
    // outTex: snapshot of outgoing scene; incoming instance is provided so it can initialize
    // during the transition start (so heavy work happens while animation runs)
    TransitionScene(RenderTexture2D outTex, std::unique_ptr<Scene> incomingInstance, float durationSeconds = 2.0f);
    ~TransitionScene();

    void Init() override;
    void Update() override;
    void Draw() override;
    void Unload() override;

    // After the transition finishes, SceneManager can take ownership of the instantiated incoming scene
    std::unique_ptr<Scene> ReleaseIncoming();

private:
    RenderTexture2D outTexture;
    RenderTexture2D inTexture;
    bool inTextureReady = false;
    std::unique_ptr<Scene> incomingInstance;

    // star expansion data
    static const int STAR_COUNT = 200;
    std::vector<Vector3> stars;
    float elapsed = 0.0f;
    float duration = 1.0f;
    bool incomingInitialized = false;
};

} // namespace Hotones
