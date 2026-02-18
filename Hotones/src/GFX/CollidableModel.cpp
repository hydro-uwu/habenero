#include <GFX/CollidableModel.hpp>
#include "AssetPath.hpp"
#include <raylib.h>
#include <raymath.h>
#include <GFX/bsp.hpp>
#if defined(_WIN32)
#include <crtdbg.h>
#endif
#include <Physics/PhysicsSystem.hpp>

namespace Hotones {

CollidableModel::CollidableModel(const std::string& path, Vector3 position)
    : position(position)
{
    std::string resolved = ResolveAssetPath(path);
    const char* loadPath = (!resolved.empty() ? resolved.c_str() : path.c_str());
    TraceLog(LOG_INFO, "CollidableModel: loading model '%s'", loadPath);
    // Check file exists before calling into raylib's loaders (can crash if file missing/corrupt)
    FILE *f = fopen(loadPath, "rb");
    if (!f) {
        TraceLog(LOG_ERROR, "CollidableModel: model file not found: %s", loadPath);
        model = {0};
    } else {
        fclose(f);
        // If the file appears to be a BSP, attempt to load via the BSP importer
        try {
            std::filesystem::path p(loadPath);
            std::string ext = p.extension().string();
            for (auto &c : ext) c = (char)tolower(c);
            // if (ext == ".bsp") {
            //     auto models = LoadModelsFromBSPFile(p);
            //     if (!models.empty()) {
            //         // Use the first generated model and unload any extras
            //         model = std::move(models[0]);
            //         for (size_t i = 1; i < models.size(); ++i) {
            //             UnloadModel(models[i]);
            //         }
            //     } else {
            //         TraceLog(LOG_ERROR, "CollidableModel: failed to import BSP: %s", loadPath);
            //         model = {0};
            //     }
            // } else {
            model = LoadModel(loadPath);
            
            
        } catch (const std::exception &e) {
            TraceLog(LOG_ERROR, "CollidableModel: exception while loading model: %s: %s", loadPath, e.what());
            model = {0};
        } catch (...) {
            TraceLog(LOG_ERROR, "CollidableModel: unknown exception while loading model: %s", loadPath);
            model = {0};
        }
    }
    if (model.meshCount <= 0 || model.meshes == NULL) {
        TraceLog(LOG_WARNING, "CollidableModel: loaded model has no meshes or failed to load meshes (meshes=%p, meshCount=%d)", (const void*)model.meshes, model.meshCount);
    }
    UpdateBoundingBox();

    // Register this model's geometry with the physics system (best-effort).
    physicsHandle = -1;
    physicsHandle = Hotones::Physics::RegisterStaticMeshFromModel(model, position);
}

CollidableModel::~CollidableModel() {
    TraceLog(LOG_INFO, "CollidableModel: unloading model (meshes=%p, meshCount=%d, materials=%p, materialCount=%d)",
             (const void*)model.meshes, model.meshCount, (const void*)model.materials, model.materialCount);
    // Only call UnloadModel if there is something to unload or pointers are non-null
    if (model.meshCount > 0 || model.materialCount > 0 || model.meshes != NULL || model.materials != NULL) {
#if defined(_WIN32)
        // Check CRT heap for corruption before unload (debug builds)
        _CrtCheckMemory();
#endif
        UnloadModel(model);
#if defined(_WIN32)
        // Check CRT heap after unload to catch overruns early
        _CrtCheckMemory();
#endif
        // Clear the model struct so dangling pointers are not reused
        model = {0};
    }
    // Unregister from physics system if needed
    if (physicsHandle != -1) {
        Hotones::Physics::UnregisterStaticMesh(physicsHandle);
        physicsHandle = -1;
    }
}

void CollidableModel::Draw() {
    DrawModel(model, position, 1.0f, WHITE);
}

void CollidableModel::SetPosition(Vector3 pos) {
    position = pos;
    UpdateBoundingBox();
}

Vector3 CollidableModel::GetPosition() const {
    return position;
}

BoundingBox CollidableModel::GetBoundingBox() const {
    return bbox;
}
bool CollidableModel::CheckCollision(const BoundingBox& other) const {
    return CheckCollisionBoxes(bbox, other);
}

bool CollidableModel::CheckCollision(Vector3 point) const {
    return (point.x >= bbox.min.x && point.x <= bbox.max.x) &&
           (point.y >= bbox.min.y && point.y <= bbox.max.y) &&
           (point.z >= bbox.min.z && point.z <= bbox.max.z);
}

bool CollidableModel::ResolveSphereCollision(Vector3 &center, float radius) {
    if (physicsHandle == -1) return false;
    return Hotones::Physics::ResolveSphereAgainstStatic(physicsHandle, center, radius);
}

bool Hotones::CollidableModel::SweepSphere(const Vector3 &start, const Vector3 &end,
                                           float radius,
                                           Vector3 &hitPos, Vector3 &hitNormal, float &t) {
    lastSweepStart = start;
    lastSweepEnd   = end;

    if (physicsHandle == -1) {
        lastSweepHit = false;
        lastSweepT   = 0.f;
        return false;
    }

    bool hit = Hotones::Physics::SweepSphereAgainstStatic(
        physicsHandle, start, end, radius, hitPos, hitNormal, t);

    lastSweepHit = hit;
    if (hit) {
        lastSweepHitPos    = hitPos;
        lastSweepHitNormal = hitNormal;
        lastSweepT         = t;
    } else {
        lastSweepT = 0.f;
    }
    return hit;
}

void Hotones::CollidableModel::DrawDebug() const {
    // draw per-mesh AABBs
    if (model.meshCount > 0 && model.meshes != NULL) {
        for (int i = 0; i < model.meshCount; ++i) {
            BoundingBox mb = GetMeshBoundingBox(model.meshes[i]);
            mb.min = Vector3Add(mb.min, position);
            mb.max = Vector3Add(mb.max, position);
            DrawBoundingBox(mb, Fade(YELLOW, 0.5f));
            DrawCubeWires((Vector3){ (mb.min.x+mb.max.x)/2.0f, (mb.min.y+mb.max.y)/2.0f, (mb.min.z+mb.max.z)/2.0f },
                          mb.max.x - mb.min.x, mb.max.y - mb.min.y, mb.max.z - mb.min.z, ORANGE);
        }
    }

    if (lastSweepHit) {
        // Draw sweep line
        DrawLine3D(lastSweepStart, lastSweepEnd, YELLOW);
        // Draw hit position and normal
        DrawSphere(lastSweepHitPos, 0.1f, GREEN);
        DrawLine3D(lastSweepHitPos, Vector3Add(lastSweepHitPos, Vector3Scale(lastSweepHitNormal, 0.5f)), BLUE);
    } else {
        // draw attempted sweep line
        DrawLine3D(lastSweepStart, lastSweepEnd, Fade(YELLOW, 0.25f));
    }
}

void Hotones::CollidableModel::DrawMeshBoundingBoxes(Color color) const {
    if (model.meshCount > 0 && model.meshes != NULL) {
        for (int i = 0; i < model.meshCount; ++i) {
            BoundingBox mb = GetMeshBoundingBox(model.meshes[i]);
            mb.min = Vector3Add(mb.min, position);
            mb.max = Vector3Add(mb.max, position);
            DrawBoundingBox(mb, Fade(color, 0.9f));
            DrawCubeWires((Vector3){ (mb.min.x+mb.max.x)/2.0f, (mb.min.y+mb.max.y)/2.0f, (mb.min.z+mb.max.z)/2.0f },
                          mb.max.x - mb.min.x, mb.max.y - mb.min.y, mb.max.z - mb.min.z, color);
        }
    }
}

void CollidableModel::UpdateBoundingBox() {
    // Compute union of all mesh bounding boxes and offset by model position
    if (model.meshCount > 0 && model.meshes != NULL) {
        BoundingBox accum = GetMeshBoundingBox(model.meshes[0]);
        for (int i = 1; i < model.meshCount; ++i) {
            BoundingBox mb = GetMeshBoundingBox(model.meshes[i]);
            // expand accum to include mb
            accum.min.x = fminf(accum.min.x, mb.min.x);
            accum.min.y = fminf(accum.min.y, mb.min.y);
            accum.min.z = fminf(accum.min.z, mb.min.z);
            accum.max.x = fmaxf(accum.max.x, mb.max.x);
            accum.max.y = fmaxf(accum.max.y, mb.max.y);
            accum.max.z = fmaxf(accum.max.z, mb.max.z);
        }
        // apply model position offset
        Vector3 offset = position;
        // Debug: print meshCount and computed accum bbox
        TraceLog(LOG_INFO, "CollidableModel: UpdateBoundingBox meshes=%d accum.min=(%f,%f,%f) accum.max=(%f,%f,%f)",
                 model.meshCount, accum.min.x, accum.min.y, accum.min.z, accum.max.x, accum.max.y, accum.max.z);
        accum.min = Vector3Add(accum.min, offset);
        accum.max = Vector3Add(accum.max, offset);
        bbox = accum;
    }
    else {
        // Fallback: empty bounding box at model position
        bbox.min = position;
        bbox.max = position;
    }
}

} // namespace Hotones
