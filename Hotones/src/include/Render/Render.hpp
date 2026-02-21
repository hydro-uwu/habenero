#pragma once

// ── Hotones::Render — C++ convenience API ────────────────────────────────────
//
// Mirrors the Lua `render.*` table exactly.
//
// Usage:
//   #include <Render/Render.hpp>
//
//   Hotones::Render::ClearScreen(30, 30, 30);
//   Hotones::Render::DrawText("Hello!", 10, 10, 24, 255, 255, 0);
//   Hotones::Render::DrawRect(50, 50, 200, 40, 80, 160, 255);

#include <GFX/Renderer.hpp>
#include <string>

namespace Hotones::Render {

/// Clear the background with the given RGBA components (0–255).
inline void ClearScreen(int r = 0, int g = 0, int b = 0, int a = 255)
{
    GFX::Renderer::ClearScreen(r, g, b, a);
}

/// Draw text with the default font.
/// r/g/b/a are 0–255; fontSize defaults to 20 px.
inline void DrawText(const std::string& text,
                     int x, int y,
                     int fontSize = 20,
                     int r = 255, int g = 255, int b = 255, int a = 255)
{
    GFX::Renderer::DrawText(text, x, y, fontSize, r, g, b, a);
}

/// Draw a filled 2-D rectangle.
inline void DrawRect(int x, int y, int w, int h,
                     int r = 255, int g = 255, int b = 255, int a = 255)
{
    GFX::Renderer::DrawRect(x, y, w, h, r, g, b, a);
}

} // namespace Hotones::Render
