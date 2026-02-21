#pragma once

// ── Hotones::Lighting — C++ convenience API ──────────────────────────────────
//
// Mirrors the Lua `lighting.*` table exactly.
// Colour parameters use the same 0-255 integer convention as the Lua API;
// they are converted to linear 0-1 internally.
//
// Usage:
//   #include <Lighting/Lighting.hpp>
//
//   Hotones::Lighting::SetAmbient(30, 30, 60, 0.2f);
//   int sun = Hotones::Lighting::Add(Hotones::Lighting::Directional,
//                                    0, 10, 0,   255, 240, 200, 1.2f);
//   Hotones::Lighting::SetDir(sun, -0.3f, -1.f, -0.5f);

#include <GFX/LightingSystem.hpp>
#include <cmath>

namespace Hotones::Lighting {

// Light-type aliases so callers don't need to reach into GFX::.
inline constexpr auto Point       = GFX::LightType::Point;
inline constexpr auto Directional = GFX::LightType::Directional;
inline constexpr auto Spot        = GFX::LightType::Spot;

/// Set the scene-wide ambient colour+intensity.
/// r, g, b are 0–255.
inline void SetAmbient(int r, int g, int b, float intensity = 0.15f)
{
    GFX::LightingSystem::Get().SetAmbient(
        { r / 255.f, g / 255.f, b / 255.f }, intensity);
}

/// Allocate a new light. Returns a 1-based handle (0 = out of slots).
/// r, g, b are 0–255.
inline int Add(GFX::LightType type,
               float x, float y, float z,
               int r, int g, int b,
               float intensity = 1.0f, float range = 20.0f)
{
    return GFX::LightingSystem::Get().AddLight(
        type, { x, y, z }, { 0.f, -1.f, 0.f },
        { r / 255.f, g / 255.f, b / 255.f }, intensity, range);
}

/// Free the light slot.
inline void Remove(int handle)
{
    GFX::LightingSystem::Get().RemoveLight(handle);
}

/// Direct access to the descriptor for fine-grained changes.
/// Returns nullptr if the handle is invalid.
inline GFX::LightDesc* Get(int handle)
{
    return GFX::LightingSystem::Get().GetLight(handle);
}

inline void SetPos(int handle, float x, float y, float z)
{
    auto* l = GFX::LightingSystem::Get().GetLight(handle);
    if (l) l->position = { x, y, z };
}

inline void SetDir(int handle, float x, float y, float z)
{
    auto* l = GFX::LightingSystem::Get().GetLight(handle);
    if (l) l->direction = { x, y, z };
}

/// r, g, b are 0–255.
inline void SetColor(int handle, int r, int g, int b)
{
    auto* l = GFX::LightingSystem::Get().GetLight(handle);
    if (l) l->color = { r / 255.f, g / 255.f, b / 255.f };
}

inline void SetIntensity(int handle, float v)
{
    auto* l = GFX::LightingSystem::Get().GetLight(handle);
    if (l) l->intensity = v;
}

inline void SetRange(int handle, float v)
{
    auto* l = GFX::LightingSystem::Get().GetLight(handle);
    if (l) l->range = v;
}

inline void SetEnabled(int handle, bool v)
{
    auto* l = GFX::LightingSystem::Get().GetLight(handle);
    if (l) l->enabled = v;
}

/// Set spot cone angles in degrees.
inline void SetSpotAngles(int handle, float innerDeg, float outerDeg)
{
    auto* l = GFX::LightingSystem::Get().GetLight(handle);
    if (l) {
        l->innerCos = cosf(innerDeg * DEG2RAD);
        l->outerCos = cosf(outerDeg * DEG2RAD);
    }
}

} // namespace Hotones::Lighting
