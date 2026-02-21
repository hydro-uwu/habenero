#pragma once

// ── Hotones::Draw3D — C++ convenience API ────────────────────────────────────
//
// Mirrors the Lua `mesh.*` table exactly.
// Must only be called from within a BeginMode3D() / EndMode3D() block
// (i.e. from a draw3D callback).
//
// Usage:
//   #include <Draw3D/Draw3D.hpp>
//
//   Hotones::Draw3D::Box(0, 0.5f, 0,  1, 1, 1,  {200, 80, 80, 255});
//   Hotones::Draw3D::Sphere(5, 1, 0,  0.5f);
//   Hotones::Draw3D::Grid(20, 1.0f);
//   Hotones::Draw3D::Axes(0, 0, 0, 2.0f);

#include <raylib.h>

namespace Hotones::Draw3D {

/// Flat horizontal plane centred at (x,y,z) with given width and depth.
inline void Plane(float x, float y, float z,
                  float width, float depth,
                  Color c = { 100, 180, 100, 255 })
{
    DrawPlane({ x, y, z }, { width, depth }, c);
}

/// Solid cube.
inline void Box(float x, float y, float z,
                float w, float h, float d,
                Color c = WHITE)
{
    DrawCubeV({ x, y, z }, { w, h, d }, c);
}

/// Wireframe cube.
inline void BoxWires(float x, float y, float z,
                     float w, float h, float d,
                     Color c = { 200, 200, 200, 255 })
{
    DrawCubeWiresV({ x, y, z }, { w, h, d }, c);
}

/// UV sphere.
inline void Sphere(float x, float y, float z, float radius,
                   int rings = 16, int slices = 16,
                   Color c = WHITE)
{
    DrawSphereEx({ x, y, z }, radius, rings, slices, c);
}

/// Cylinder/cone from (x,y,z) upwards by `height`.
inline void Cylinder(float x, float y, float z,
                     float radiusTop, float radiusBottom, float height,
                     int slices = 16, Color c = WHITE)
{
    DrawCylinderEx({ x, y, z }, { x, y + height, z }, radiusBottom, radiusTop, slices, c);
}

/// 3-D line segment.
inline void Line(float x1, float y1, float z1,
                 float x2, float y2, float z2,
                 Color c = WHITE)
{
    DrawLine3D({ x1, y1, z1 }, { x2, y2, z2 }, c);
}

/// Flat grid on the XZ plane.
inline void Grid(int slices = 20, float spacing = 1.0f)
{
    DrawGrid(slices, spacing);
}

/// Three axis lines: X=red, Y=green, Z=blue.
inline void Axes(float x = 0.f, float y = 0.f, float z = 0.f, float size = 1.0f)
{
    DrawLine3D({ x, y, z }, { x + size, y,        z        }, RED);
    DrawLine3D({ x, y, z }, { x,        y + size,  z        }, GREEN);
    DrawLine3D({ x, y, z }, { x,        y,         z + size }, BLUE);
}

} // namespace Hotones::Draw3D
