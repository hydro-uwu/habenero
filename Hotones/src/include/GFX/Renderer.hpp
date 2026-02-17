#pragma once

#include <string>

struct Color;

namespace Hotones::GFX {

class Renderer {
public:
    // Clear background with RGBA components (0-255)
    static void ClearScreen(int r, int g, int b, int a = 255);

    // Draw text using the default font
    static void DrawText(const std::string &text, int x, int y, int fontSize, int r, int g, int b, int a = 255);

    // Draw a filled rectangle
    static void DrawRect(int x, int y, int w, int h, int r, int g, int b, int a = 255);
};

} // namespace Hotones::GFX
