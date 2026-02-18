#pragma once
#include <raylib.h>
#include <raymath.h>
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <variant>
#include <optional>
#include <memory>

// ─── SceneImporter ────────────────────────────────────────────────────────────
//
// Loads GLTF/GLB (and any other Assimp-supported format) and exposes the full
// scene graph: named nodes, transforms, per-node custom properties (GLTF
// "extras"), lights, and raylib-compatible render meshes — all in one pass.
//
// Usage:
//   auto scene = Hotones::SceneImporter::Load("assets/level.glb");
//   if (scene) {
//       scene->Draw();
//       auto* lamp = scene->GetNode("Lamp.001");
//       if (lamp) { ... lamp->lights, lamp->properties ... }
//   }

namespace Hotones {

// ─── Property bag (GLTF extras / aiMetadata) ─────────────────────────────────

using PropValue = std::variant<
    bool,
    int64_t,
    double,
    std::string,
    Vector3,
    Vector4
>;

struct PropertyBag {
    std::unordered_map<std::string, PropValue> data;

    bool              Has(const std::string& key) const;
    std::string       GetString (const std::string& key, const std::string& def = "") const;
    double            GetFloat  (const std::string& key, double def = 0.0)             const;
    int64_t           GetInt    (const std::string& key, int64_t def = 0)              const;
    bool              GetBool   (const std::string& key, bool def = false)             const;
    std::optional<Vector3>  GetVec3(const std::string& key) const;
};

// ─── Light ───────────────────────────────────────────────────────────────────

enum class SceneLightType { Point, Directional, Spot, Area };

struct SceneLight {
    std::string     name;
    SceneLightType  type        = SceneLightType::Point;
    Vector3         position    = {0,0,0};   // world-space
    Vector3         direction   = {0,-1,0};  // world-space, normalised
    Color           color       = WHITE;
    float           intensity   = 1.0f;
    float           range       = 10.0f;     // point / spot attenuation radius
    float           innerAngle  = 0.f;       // spot inner cone (radians)
    float           outerAngle  = 0.5f;      // spot outer cone (radians)
    PropertyBag     properties;
};

// ─── Scene node ──────────────────────────────────────────────────────────────

struct SceneNode {
    std::string              name;
    Matrix                   transform = MatrixIdentity(); // world-space
    std::vector<std::string> meshNames;  // which render meshes belong here
    PropertyBag              properties; // GLTF extras / aiMetadata
    std::vector<int>         children;   // indices into ImportedScene::nodes
    int                      parent = -1;
};

// ─── Per-mesh render info ─────────────────────────────────────────────────────

struct SceneMesh {
    std::string name;
    Mesh        mesh    = {0};   // raylib Mesh (uploaded to GPU)
    Material    mat     = {0};   // raylib Material
    Matrix      transform = MatrixIdentity(); // node world transform at import time
    int         physicsHandle = -1;          // -1 = not registered
};

// ─── Imported scene ──────────────────────────────────────────────────────────

struct ImportedScene {
    std::string             path;
    std::vector<SceneMesh>  meshes;
    std::vector<SceneNode>  nodes;
    std::vector<SceneLight> lights;
    std::vector<int>        rootNodes; // indices of top-level nodes

    // ── Rendering ──────────────────────────────────────────────────────────

    // Draw all visible meshes (applies per-mesh transform).
    void Draw() const;
    // Draw with an override tint.
    void DrawTinted(Color tint) const;

    // ── Node queries ───────────────────────────────────────────────────────

    // Find first node whose name equals `name` (case-sensitive). Returns nullptr if not found.
    const SceneNode* GetNode(const std::string& name) const;
          SceneNode* GetNode(const std::string& name);

    // Find all nodes that have a given property key set.
    std::vector<const SceneNode*> FindNodesByProperty(const std::string& key) const;
    // Find all nodes whose name contains `substr`.
    std::vector<const SceneNode*> FindNodesByName(const std::string& substr) const;

    // ── Light queries ──────────────────────────────────────────────────────
    const std::vector<SceneLight>& GetLights() const { return lights; }

    // ── Lifecycle ──────────────────────────────────────────────────────────
    void Unload();
};

// ─── Importer ────────────────────────────────────────────────────────────────

struct SceneImportOptions {
    bool registerPhysics = true;   // register mesh triangles with PhysicsSystem
    bool flipUVs         = true;   // flip V coord (OpenGL convention)
    bool generateNormals = true;   // generate smooth normals if missing
    bool mergeByMaterial = false;  // merge meshes that share a material
    float scale          = 1.0f;   // uniform scale applied at load time
};

class SceneImporter {
public:
    // Load a file using Assimp.  Returns nullptr on failure.
    // Supported formats: GLTF/GLB, OBJ, FBX, DAE, 3DS, …
    static std::unique_ptr<ImportedScene> Load(
        const std::string& path,
        const SceneImportOptions& opts = {});
};

} // namespace Hotones
