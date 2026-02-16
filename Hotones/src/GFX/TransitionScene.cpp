#include "TransitionScene.hpp"
#include <raymath.h>

namespace Hotones {

TransitionScene::TransitionScene(RenderTexture2D outTex, std::unique_ptr<Scene> incoming, float durationSeconds)
    : outTexture(outTex), incomingInstance(std::move(incoming)), duration(durationSeconds)
{
}

TransitionScene::~TransitionScene()
{
    // Ensure textures are unloaded if still present
    if (outTexture.id) UnloadRenderTexture(outTexture);
    if (inTexture.id) UnloadRenderTexture(inTexture);
}

void TransitionScene::Init()
{
    // Initialize a modest starfield centered around screen center
    int w = GetScreenWidth();
    int h = GetScreenHeight();
    stars.reserve(STAR_COUNT);
    for (int i = 0; i < STAR_COUNT; ++i) {
        Vector3 s = { (float)GetRandomValue(-w/2, w/2), (float)GetRandomValue(-h/2, h/2), 1.0f };
        stars.push_back(s);
    }
    elapsed = 0.0f;
    inTextureReady = false;
}

void TransitionScene::Update()
{
    float dt = GetFrameTime();
    elapsed += dt;

    // If incoming scene exists, let it progress while transition runs
    if (incomingInstance) {
        incomingInstance->Update();
        if (!inTextureReady) {
            inTexture = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
            inTextureReady = true;
        }
    }

    if (elapsed >= duration) {
        // Finish transition and mark finished
        MarkFinished();
    }
}

void TransitionScene::Draw()
{
    const int w = GetScreenWidth();
    const int h = GetScreenHeight();
    float t = Clamp(elapsed / duration, 0.0f, 1.0f);
    float half = 0.5f;
    // easing
    auto easeInOut = [](float x)->float { return (1.0f - cosf(PI * x)) * 0.5f; };

    // Layered 2.5D effect: draw multiple scaled layers of the snapshot with slight offsets
    if (t < half) {
        float phase = t / half; // 0..1
        float e = easeInOut(phase);

        // draw 3 layers of outgoing snapshot with parallax offsets
        for (int layer = 2; layer >= 0; --layer) {
            float layerOffset = 0.02f * (float)layer;
            float scale = 1.0f + (0.25f + layerOffset) * e;
            float alpha = Clamp(1.0f - 0.25f * layer, 0.2f, 1.0f);

            Rectangle src = { 0.0f, 0.0f, (float)outTexture.texture.width, (float)-outTexture.texture.height };
            Rectangle dst = { (w*(1.0f-scale))/2.0f, (h*(1.0f-scale))/2.0f, w*scale, h*scale };
            Color tint = WHITE;
            tint.a = (unsigned char)(255 * alpha);
            DrawTexturePro(outTexture.texture, src, dst, {0,0}, -5.0f * layerOffset * e * 10.0f, tint);
        }

        // star expansion (multiple depth layers)
        Vector2 center = { w*0.5f, h*0.5f };
        for (int depth = 0; depth < 3; ++depth) {
            float depthFactor = 1.0f + depth * 0.6f;
            float expand = Lerp(0.0f, 4.0f * depthFactor, e);
            float grow = 1.0f + 4.0f * e * (1.0f + depth);
            Color starColor = RAYWHITE;
            starColor.a = (unsigned char)(255 * (1.0f - 0.25f*depth));
            for (auto &s : stars) {
                Vector2 pos = { center.x + s.x * expand, center.y + s.y * expand };
                DrawCircleV(pos, grow, starColor);
            }
        }

    } else {
        float phase = (t - half) / half; // 0..1
        float e = easeInOut(phase);

        // draw incoming layers (reverse order)
        for (int layer = 2; layer >= 0; --layer) {
            float layerOffset = 0.02f * (float)layer;
            float scale = 1.25f - (0.25f + layerOffset) * e;
            float alpha = Clamp(1.0f - 0.25f * layer, 0.2f, 1.0f);

            if (inTextureReady && incomingInstance) {
                // render incoming scene to texture each frame so reveal shows live scene
                BeginTextureMode(inTexture);
                ClearBackground(BLACK);
                incomingInstance->Draw();
                EndTextureMode();

                Rectangle src = { 0.0f, 0.0f, (float)inTexture.texture.width, (float)-inTexture.texture.height };
                Rectangle dst = { (w*(1.0f-scale))/2.0f, (h*(1.0f-scale))/2.0f, w*scale, h*scale };
                Color tint = WHITE;
                tint.a = (unsigned char)(255 * alpha);
                DrawTexturePro(inTexture.texture, src, dst, {0,0}, 5.0f * layerOffset * e * 10.0f, tint);
            } else {
                ClearBackground(BLACK);
            }
        }

        // shrinking starfield to reveal incoming
        Vector2 center = { w*0.5f, h*0.5f };
        for (int depth = 0; depth < 3; ++depth) {
            float depthFactor = 1.0f + depth * 0.6f;
            float shrink = Lerp(4.0f * depthFactor, 0.0f, e);
            float grow = 1.0f + 4.0f * (1.0f - e) * (1.0f + depth);
            Color starColor = RAYWHITE;
            starColor.a = (unsigned char)(255 * (1.0f - 0.25f*depth));
            for (auto &s : stars) {
                Vector2 pos = { center.x + s.x * shrink, center.y + s.y * shrink };
                DrawCircleV(pos, grow, starColor);
            }
        }
    }
}

void TransitionScene::Unload()
{
    if (outTexture.id) { UnloadRenderTexture(outTexture); outTexture.id = 0; }
    if (inTexture.id) { UnloadRenderTexture(inTexture); inTexture.id = 0; }
    // incomingInstance intentionally not unloaded here; it will be released by ReleaseIncoming()
}

std::unique_ptr<Scene> TransitionScene::ReleaseIncoming()
{
    return std::move(incomingInstance);
}

} // namespace Hotones
// (duplicate/old implementation removed — single implementation retained above)
