#include "../include/GFX/Renderer.hpp"
#include <raylib.h>

namespace Hotones::GFX {

void Renderer::ClearScreen(int r, int g, int b, int a)
{
    ClearBackground(Color{(unsigned char)r, (unsigned char)g, (unsigned char)b, (unsigned char)a});
}

void Renderer::DrawText(const std::string &text, int x, int y, int fontSize, int r, int g, int b, int a)
{
    DrawTextEx(GetFontDefault(), text.c_str(), Vector2{(float)x, (float)y}, (float)fontSize, 0.0f, Color{(unsigned char)r, (unsigned char)g, (unsigned char)b, (unsigned char)a});
}

void Renderer::DrawRect(int x, int y, int w, int h, int r, int g, int b, int a)
{
    DrawRectangle(x, y, w, h, Color{(unsigned char)r, (unsigned char)g, (unsigned char)b, (unsigned char)a});
}

} // namespace Hotones::GFX
