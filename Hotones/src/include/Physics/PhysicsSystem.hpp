#pragma once
#include <raylib.h>

namespace Hotones { namespace Physics {

// Initialize the physics subsystem. Returns true on success.
bool InitPhysics();
void ShutdownPhysics();

// Register a static (non-moving) collision mesh built from a raylib `Model`.
// Returns a positive handle id on success, or -1 if registration failed / not available.
int RegisterStaticMeshFromModel(const Model& model, const Vector3& position);
void UnregisterStaticMesh(int handle);

// Continuous sphere sweep against a registered static mesh.
// start/end are sphere center positions. Returns true if hit; t ∈ [0,1].
bool SweepSphereAgainstStatic(int handle, const Vector3& start, const Vector3& end,
                               float radius,
                               Vector3& hitPos, Vector3& hitNormal, float& t);

// Discrete sphere penetration resolve: pushes `center` out of all overlapping
// triangles in one pass. Returns true if any triangle was overlapping.
bool ResolveSphereAgainstStatic(int handle, Vector3& center, float radius);

}} // namespace Hotones::Physics
