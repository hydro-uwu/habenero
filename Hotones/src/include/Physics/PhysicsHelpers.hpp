#pragma once

// ── Hotones::Physics — result-struct helpers ──────────────────────────────────
//
// Wraps the raw out-parameter APIs in PhysicsSystem.hpp with ergonomic
// value-returning helpers that mirror Lua's physics.* return values.
//
// Usage:
//   #include <Physics/PhysicsHelpers.hpp>
//
//   auto hit = Hotones::Physics::Raycast(meshHandle,
//                                        origin, dir, 500.f);
//   if (hit) {
//       // hit.pos, hit.normal, hit.t are all populated
//   }
//
//   auto sweep = Hotones::Physics::SweepSphere(meshHandle,
//                                              start, end, 0.5f);
//   if (sweep) { ... }

#include <Physics/PhysicsSystem.hpp>
#include <raylib.h>

namespace Hotones::Physics {

/// Result of a raycast query.  Evaluates to `true` when an intersection occurred.
struct RaycastResult {
    bool    hit    = false;
    Vector3 pos    = { 0, 0, 0 };
    Vector3 normal = { 0, 1, 0 };
    float   t      = 0.f;

    explicit operator bool() const { return hit; }
};

/// Result of a sphere-sweep query.  Evaluates to `true` when an intersection occurred.
struct SweepResult {
    bool    hit    = false;
    Vector3 pos    = { 0, 0, 0 };
    Vector3 normal = { 0, 1, 0 };
    /// Fraction [0,1] along the sweep segment where contact first occurs.
    float   t      = 0.f;

    explicit operator bool() const { return hit; }
};

/// Cast a ray and return a RaycastResult by value.
/// `dir` does not need to be normalised; returned `t` is in the same units as `dir`.
inline RaycastResult Raycast(int handle,
                              const Vector3& origin,
                              const Vector3& dir,
                              float maxDist = 1000.f)
{
    RaycastResult res;
    res.hit = RaycastAgainstStatic(handle, origin, dir, maxDist,
                                   res.pos, res.normal, res.t);
    return res;
}

/// Sweep a sphere and return a SweepResult by value.
inline SweepResult SweepSphere(int handle,
                                const Vector3& start,
                                const Vector3& end,
                                float radius)
{
    SweepResult res;
    res.hit = SweepSphereAgainstStatic(handle, start, end, radius,
                                       res.pos, res.normal, res.t);
    return res;
}

} // namespace Hotones::Physics
