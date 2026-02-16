#include "SimpleScene.hpp"

namespace Hotones {

SimpleScene::SimpleScene(const char* label, float durationSeconds)
    : text(label), elapsed(0.0f), duration(durationSeconds)
{
}

void SimpleScene::Init() {}

void SimpleScene::Update()
{
    elapsed += GetFrameTime();
    if (elapsed >= duration) MarkFinished();
}

void SimpleScene::Draw()
{
    ClearBackground(RAYWHITE);
    DrawText(text, GetScreenWidth()/2 - MeasureText(text, 40)/2, GetScreenHeight()/2 - 20, 40, BLACK);
}

} // namespace Hotones
