// SceneImporter.cpp — Assimp-backed scene loader
//
// Converts an Assimp aiScene into:
//   • Raylib Mesh / Material pairs  (rendering)
//   • A flat node list with names, world transforms, GLTF extras  (gameplay)
//   • SceneLight list  (lighting)
//   • Per-mesh BVH handles registered with PhysicsSystem  (collision)
//
// Assimp post-process flags used:
//   aiProcess_Triangulate           — ensure every face is a triangle
//   aiProcess_GenSmoothNormals      — generate normals if missing
//   aiProcess_CalcTangentSpace      — tangents for normal mapping
//   aiProcess_JoinIdenticalVertices — reduce vertex count
//   aiProcess_FlipUVs               — match OpenGL (0,0 = bottom-left)
//   aiProcess_GlobalScale           — applies AI_CONFIG_GLOBAL_SCALE_FACTOR
//   aiProcess_PopulateArmatureData  — bone data (unused for now, benign)

#include <GFX/SceneImporter.hpp>
#include <Physics/PhysicsSystem.hpp>
#include "AssetPath.hpp"

// Assimp
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/material.h>
#include <assimp/GltfMaterial.h>

#include <raylib.h>
#include <raymath.h>

#include <algorithm>
#include <cassert>
#include <cstring>
#include <iostream>
#include <string>
#include <unordered_map>

// ─── Internal helpers ─────────────────────────────────────────────────────────

static inline Vector3 Ai2Rl(const aiVector3D& v) { return { v.x, v.y, v.z }; }
static inline Color   Ai2Clr(const aiColor3D& c, float intensity = 1.f) {
    return {
        (unsigned char)Clamp(c.r * intensity * 255.f, 0.f, 255.f),
        (unsigned char)Clamp(c.g * intensity * 255.f, 0.f, 255.f),
        (unsigned char)Clamp(c.b * intensity * 255.f, 0.f, 255.f),
        255
    };
}

// Convert Assimp row-major 4×4 matrix to raylib column-major Matrix.
// Assimp stores matrices row-major (a[row][col]); raylib/OpenGL is column-major.
static Matrix Ai2Matrix(const aiMatrix4x4& m) {
    // Assimp: m.a1..a4 = row 0 cols 0..3
    return {
        m.a1, m.b1, m.c1, m.d1,
        m.a2, m.b2, m.c2, m.d2,
        m.a3, m.b3, m.c3, m.d3,
        m.a4, m.b4, m.c4, m.d4
    };
}

// ─── PropertyBag implementation ───────────────────────────────────────────────

namespace Hotones {

bool PropertyBag::Has(const std::string& key) const {
    return data.count(key) > 0;
}
std::string PropertyBag::GetString(const std::string& key, const std::string& def) const {
    auto it = data.find(key);
    if (it == data.end()) return def;
    if (auto* p = std::get_if<std::string>(&it->second)) return *p;
    return def;
}
double PropertyBag::GetFloat(const std::string& key, double def) const {
    auto it = data.find(key);
    if (it == data.end()) return def;
    if (auto* p = std::get_if<double>(&it->second))  return *p;
    if (auto* p = std::get_if<int64_t>(&it->second)) return (double)*p;
    if (auto* p = std::get_if<bool>(&it->second))    return *p ? 1.0 : 0.0;
    return def;
}
int64_t PropertyBag::GetInt(const std::string& key, int64_t def) const {
    auto it = data.find(key);
    if (it == data.end()) return def;
    if (auto* p = std::get_if<int64_t>(&it->second)) return *p;
    if (auto* p = std::get_if<double>(&it->second))  return (int64_t)*p;
    if (auto* p = std::get_if<bool>(&it->second))    return *p ? 1 : 0;
    return def;
}
bool PropertyBag::GetBool(const std::string& key, bool def) const {
    auto it = data.find(key);
    if (it == data.end()) return def;
    if (auto* p = std::get_if<bool>(&it->second))    return *p;
    if (auto* p = std::get_if<int64_t>(&it->second)) return *p != 0;
    if (auto* p = std::get_if<double>(&it->second))  return *p != 0.0;
    return def;
}
std::optional<Vector3> PropertyBag::GetVec3(const std::string& key) const {
    auto it = data.find(key);
    if (it == data.end()) return std::nullopt;
    if (auto* p = std::get_if<Vector3>(&it->second)) return *p;
    if (auto* p = std::get_if<Vector4>(&it->second)) return Vector3{p->x, p->y, p->z};
    return std::nullopt;
}

// ─── ImportedScene implementation ────────────────────────────────────────────

void ImportedScene::Draw() const {
    for (const auto& sm : meshes)
        DrawMesh(sm.mesh, sm.mat, sm.transform);
}

void ImportedScene::DrawTinted(Color tint) const {
    for (const auto& sm : meshes) {
        Material mat = sm.mat;
        mat.maps[MATERIAL_MAP_DIFFUSE].color = tint;
        DrawMesh(sm.mesh, mat, sm.transform);
    }
}

const SceneNode* ImportedScene::GetNode(const std::string& name) const {
    for (const auto& n : nodes)
        if (n.name == name) return &n;
    return nullptr;
}
SceneNode* ImportedScene::GetNode(const std::string& name) {
    for (auto& n : nodes)
        if (n.name == name) return &n;
    return nullptr;
}
std::vector<const SceneNode*> ImportedScene::FindNodesByProperty(const std::string& key) const {
    std::vector<const SceneNode*> out;
    for (const auto& n : nodes)
        if (n.properties.Has(key)) out.push_back(&n);
    return out;
}
std::vector<const SceneNode*> ImportedScene::FindNodesByName(const std::string& substr) const {
    std::vector<const SceneNode*> out;
    for (const auto& n : nodes)
        if (n.name.find(substr) != std::string::npos) out.push_back(&n);
    return out;
}

void ImportedScene::Unload() {
    for (auto& sm : meshes) {
        UnloadMesh(sm.mesh);
        UnloadMaterial(sm.mat);
        if (sm.physicsHandle != -1) {
            Physics::UnregisterStaticMesh(sm.physicsHandle);
            sm.physicsHandle = -1;
        }
    }
    meshes.clear();
    nodes.clear();
    lights.clear();
}

// ─── Assimp → PropertyBag conversion ─────────────────────────────────────────

static PropertyBag MetadataToPropertyBag(const aiMetadata* meta) {
    PropertyBag bag;
    if (!meta) return bag;
    for (unsigned int i = 0; i < meta->mNumProperties; ++i) {
        std::string key(meta->mKeys[i].C_Str());
        const aiMetadataEntry& entry = meta->mValues[i];
        switch (entry.mType) {
            case AI_BOOL:
                bag.data[key] = PropValue{ *static_cast<bool*>(entry.mData) };
                break;
            case AI_INT32:
                bag.data[key] = PropValue{ (int64_t)*static_cast<int32_t*>(entry.mData) };
                break;
            case AI_UINT64:
                bag.data[key] = PropValue{ (int64_t)*static_cast<uint64_t*>(entry.mData) };
                break;
            case AI_FLOAT:
                bag.data[key] = PropValue{ (double)*static_cast<float*>(entry.mData) };
                break;
            case AI_DOUBLE:
                bag.data[key] = PropValue{ *static_cast<double*>(entry.mData) };
                break;
            case AI_AISTRING:
                bag.data[key] = PropValue{ std::string(static_cast<aiString*>(entry.mData)->C_Str()) };
                break;
            case AI_AIVECTOR3D: {
                const aiVector3D* v = static_cast<aiVector3D*>(entry.mData);
                bag.data[key] = PropValue{ Vector3{v->x, v->y, v->z} };
                break;
            }
            default:
                break;
        }
    }
    return bag;
}

// ─── Assimp mesh → raylib Mesh ────────────────────────────────────────────────

static Mesh AiMeshToRaylibMesh(const aiMesh* aim) {
    Mesh m = {0};
    if (!aim || aim->mNumVertices == 0) return m;

    m.vertexCount  = (int)aim->mNumVertices;
    m.triangleCount = (int)aim->mNumFaces;

    // Positions (required)
    m.vertices = (float*)MemAlloc(m.vertexCount * 3 * sizeof(float));
    for (int i = 0; i < m.vertexCount; ++i) {
        m.vertices[i*3+0] = aim->mVertices[i].x;
        m.vertices[i*3+1] = aim->mVertices[i].y;
        m.vertices[i*3+2] = aim->mVertices[i].z;
    }

    // Normals
    if (aim->HasNormals()) {
        m.normals = (float*)MemAlloc(m.vertexCount * 3 * sizeof(float));
        for (int i = 0; i < m.vertexCount; ++i) {
            m.normals[i*3+0] = aim->mNormals[i].x;
            m.normals[i*3+1] = aim->mNormals[i].y;
            m.normals[i*3+2] = aim->mNormals[i].z;
        }
    }

    // UV channel 0
    if (aim->HasTextureCoords(0)) {
        m.texcoords = (float*)MemAlloc(m.vertexCount * 2 * sizeof(float));
        for (int i = 0; i < m.vertexCount; ++i) {
            m.texcoords[i*2+0] = aim->mTextureCoords[0][i].x;
            m.texcoords[i*2+1] = aim->mTextureCoords[0][i].y;
        }
    }

    // Tangents (for normal mapping)
    if (aim->HasTangentsAndBitangents()) {
        m.tangents = (float*)MemAlloc(m.vertexCount * 4 * sizeof(float));
        for (int i = 0; i < m.vertexCount; ++i) {
            m.tangents[i*4+0] = aim->mTangents[i].x;
            m.tangents[i*4+1] = aim->mTangents[i].y;
            m.tangents[i*4+2] = aim->mTangents[i].z;
            m.tangents[i*4+3] = 1.0f; // handedness
        }
    }

    // Vertex colors (channel 0)
    if (aim->HasVertexColors(0)) {
        m.colors = (unsigned char*)MemAlloc(m.vertexCount * 4 * sizeof(unsigned char));
        for (int i = 0; i < m.vertexCount; ++i) {
            m.colors[i*4+0] = (unsigned char)(aim->mColors[0][i].r * 255.f);
            m.colors[i*4+1] = (unsigned char)(aim->mColors[0][i].g * 255.f);
            m.colors[i*4+2] = (unsigned char)(aim->mColors[0][i].b * 255.f);
            m.colors[i*4+3] = (unsigned char)(aim->mColors[0][i].a * 255.f);
        }
    }

    // Indices (unsigned short; raylib uses 16-bit indices)
    m.indices = (unsigned short*)MemAlloc(m.triangleCount * 3 * sizeof(unsigned short));
    for (int f = 0; f < m.triangleCount; ++f) {
        assert(aim->mFaces[f].mNumIndices == 3);
        m.indices[f*3+0] = (unsigned short)aim->mFaces[f].mIndices[0];
        m.indices[f*3+1] = (unsigned short)aim->mFaces[f].mIndices[1];
        m.indices[f*3+2] = (unsigned short)aim->mFaces[f].mIndices[2];
    }

    UploadMesh(&m, false);
    return m;
}

// ─── Assimp material → raylib Material ───────────────────────────────────────

static Material AiMaterialToRaylibMaterial(const aiMaterial* aim,
                                            const std::string& basePath) {
    Material mat = LoadMaterialDefault();

    // Diffuse / base color
    aiColor4D diffuse(1,1,1,1);
    if (AI_SUCCESS == aim->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse)) {
        mat.maps[MATERIAL_MAP_DIFFUSE].color = {
            (unsigned char)(diffuse.r * 255),
            (unsigned char)(diffuse.g * 255),
            (unsigned char)(diffuse.b * 255),
            (unsigned char)(diffuse.a * 255),
        };
    }
    // PBR base color (GLTF)
    aiColor4D base(1,1,1,1);
    if (AI_SUCCESS == aim->Get(AI_MATKEY_BASE_COLOR, base)) {
        mat.maps[MATERIAL_MAP_DIFFUSE].color = {
            (unsigned char)(base.r * 255),
            (unsigned char)(base.g * 255),
            (unsigned char)(base.b * 255),
            (unsigned char)(base.a * 255),
        };
    }

    // Metallic / roughness
    float metallic = 0.f, roughness = 1.f;
    aim->Get(AI_MATKEY_METALLIC_FACTOR,  metallic);
    aim->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness);
    mat.maps[MATERIAL_MAP_ROUGHNESS].value = roughness;
    mat.maps[MATERIAL_MAP_METALNESS].value = metallic;

    // Emissive
    aiColor3D emissive(0,0,0);
    if (AI_SUCCESS == aim->Get(AI_MATKEY_COLOR_EMISSIVE, emissive)) {
        mat.maps[MATERIAL_MAP_EMISSION].color = {
            (unsigned char)(emissive.r * 255),
            (unsigned char)(emissive.g * 255),
            (unsigned char)(emissive.b * 255),
            255
        };
    }

    auto loadTex = [&](aiTextureType type, int mapIndex) {
        aiString texPath;
        if (AI_SUCCESS == aim->GetTexture(type, 0, &texPath)) {
            std::string tp(texPath.C_Str());
            // Assimp embedded textures start with '*'
            if (!tp.empty() && tp[0] != '*') {
                std::string full = basePath + "/" + tp;
                if (FileExists(full.c_str())) {
                    mat.maps[mapIndex].texture = LoadTexture(full.c_str());
                }
            }
        }
    };

    loadTex(aiTextureType_DIFFUSE,     MATERIAL_MAP_DIFFUSE);
    loadTex(aiTextureType_BASE_COLOR,  MATERIAL_MAP_DIFFUSE);
    loadTex(aiTextureType_NORMALS,     MATERIAL_MAP_NORMAL);
    loadTex(aiTextureType_EMISSIVE,    MATERIAL_MAP_EMISSION);
    loadTex(aiTextureType_METALNESS,   MATERIAL_MAP_METALNESS);
    loadTex(aiTextureType_DIFFUSE_ROUGHNESS, MATERIAL_MAP_ROUGHNESS);
    loadTex(aiTextureType_AMBIENT_OCCLUSION, MATERIAL_MAP_OCCLUSION);

    return mat;
}

// ─── Light extraction ─────────────────────────────────────────────────────────

static SceneLight ExtractLight(const aiLight* alight, const aiScene* scene) {
    SceneLight sl;
    sl.name = alight->mName.C_Str();

    // Determine world-space position/direction by finding the node for this light
    Matrix worldTm = MatrixIdentity();
    const aiNode* lightNode = scene->mRootNode->FindNode(alight->mName);
    if (lightNode) {
        // Accumulate transforms up to root
        aiMatrix4x4 tm = lightNode->mTransformation;
        const aiNode* cur = lightNode->mParent;
        while (cur) { tm = cur->mTransformation * tm; cur = cur->mParent; }
        worldTm = Ai2Matrix(tm);
    }

    Vector3 localPos = Ai2Rl(alight->mPosition);
    Vector3 localDir = Ai2Rl(alight->mDirection);
    sl.position  = Vector3Transform(localPos, worldTm);
    // Direction: apply only rotation part (no translation)
    Matrix rotOnly = worldTm;
    rotOnly.m12 = rotOnly.m13 = rotOnly.m14 = 0.f;
    sl.direction = Vector3Normalize(Vector3Transform(localDir, rotOnly));

    // Intensity: use the max channel of the color attenuated colour
    aiColor3D col = alight->mColorDiffuse;
    sl.intensity = fmaxf(fmaxf(col.r, col.g), col.b);
    float invI = sl.intensity > 1e-5f ? 1.f / sl.intensity : 1.f;
    sl.color = Ai2Clr({col.r * invI, col.g * invI, col.b * invI}, 1.f);

    // Attenuation → derive range  (range where intensity drops to ~1%)
    float a0 = alight->mAttenuationConstant;
    float a1 = alight->mAttenuationLinear;
    float a2 = alight->mAttenuationQuadratic;
    if (a2 > 1e-6f)
        sl.range = sqrtf(100.f * sl.intensity / a2);
    else if (a1 > 1e-6f)
        sl.range = 100.f * sl.intensity / a1;
    else
        sl.range = 20.f; // default

    switch (alight->mType) {
        case aiLightSource_DIRECTIONAL: sl.type = SceneLightType::Directional; break;
        case aiLightSource_SPOT:
            sl.type       = SceneLightType::Spot;
            sl.innerAngle = alight->mAngleInnerCone;
            sl.outerAngle = alight->mAngleOuterCone;
            break;
        case aiLightSource_AREA: sl.type = SceneLightType::Area; break;
        default:                 sl.type = SceneLightType::Point; break;
    }

    // Extract any metadata on the light's node
    if (lightNode && lightNode->mMetaData)
        sl.properties = MetadataToPropertyBag(lightNode->mMetaData);

    return sl;
}

// ─── Node tree walk ───────────────────────────────────────────────────────────

struct BuildContext {
    const aiScene*        ai_scene;
    ImportedScene*        out;
    const std::string&    basePath;
    const SceneImportOptions& opts;
    // Map from Assimp mesh index → index in out->meshes
    std::unordered_map<unsigned int, int> meshIndexMap;
};

static int WalkNode(const aiNode* node, int parentIdx,
                    const aiMatrix4x4& parentTm, BuildContext& ctx) {
    // Compute world transform
    aiMatrix4x4 worldTm = parentTm * node->mTransformation;
    Matrix rlTm = Ai2Matrix(worldTm);

    SceneNode sn;
    sn.name      = node->mName.C_Str();
    sn.transform = rlTm;
    sn.parent    = parentIdx;
    sn.properties = MetadataToPropertyBag(node->mMetaData);

    int nodeIdx = (int)ctx.out->nodes.size();
    ctx.out->nodes.push_back(std::move(sn)); // placeholder, children filled below

    // Attach meshes
    for (unsigned int mi = 0; mi < node->mNumMeshes; ++mi) {
        unsigned int aimIdx = node->mMeshes[mi];
        auto it = ctx.meshIndexMap.find(aimIdx);
        if (it == ctx.meshIndexMap.end()) {
            // First time we see this Assimp mesh — convert it
            const aiMesh* aim = ctx.ai_scene->mMeshes[aimIdx];

            SceneMesh sm;
            sm.name      = aim->mName.C_Str();
            sm.mesh      = AiMeshToRaylibMesh(aim);
            sm.transform = rlTm;
            if (ctx.ai_scene->mNumMaterials > aim->mMaterialIndex)
                sm.mat = AiMaterialToRaylibMaterial(
                    ctx.ai_scene->mMaterials[aim->mMaterialIndex], ctx.basePath);
            else
                sm.mat = LoadMaterialDefault();

            // Register physics (uses raylib mesh data we just built)
            if (ctx.opts.registerPhysics && sm.mesh.vertexCount > 0) {
                // Build a temporary single-mesh Model to pass into RegisterStaticMeshFromModel
                Model tmp = {0};
                tmp.meshCount = 1;
                tmp.meshes    = &sm.mesh;
                // Extract world position from matrix for the physics offset
                Vector3 pos = { rlTm.m12, rlTm.m13, rlTm.m14 };
                sm.physicsHandle = Physics::RegisterStaticMeshFromModel(tmp, pos);
            }

            int smIdx = (int)ctx.out->meshes.size();
            ctx.meshIndexMap[aimIdx] = smIdx;
            ctx.out->meshes.push_back(std::move(sm));
            ctx.out->nodes[nodeIdx].meshNames.push_back(
                ctx.out->meshes[smIdx].name.empty()
                    ? ("mesh_" + std::to_string(smIdx))
                    : ctx.out->meshes[smIdx].name);
        } else {
            ctx.out->nodes[nodeIdx].meshNames.push_back(
                ctx.out->meshes[it->second].name);
        }
    }

    // Recurse children
    for (unsigned int ci = 0; ci < node->mNumChildren; ++ci) {
        int childIdx = WalkNode(node->mChildren[ci], nodeIdx, worldTm, ctx);
        ctx.out->nodes[nodeIdx].children.push_back(childIdx);
    }

    return nodeIdx;
}

// ─── SceneImporter::Load ──────────────────────────────────────────────────────

std::unique_ptr<ImportedScene> SceneImporter::Load(
        const std::string& path,
        const SceneImportOptions& opts)
{
    // Resolve asset path (handles build/ prefix search, etc.)
    std::string resolved = ResolveAssetPath(path);
    const std::string& loadPath = resolved.empty() ? path : resolved;

    if (!FileExists(loadPath.c_str())) {
        TraceLog(LOG_ERROR, "SceneImporter: file not found: %s", loadPath.c_str());
        return nullptr;
    }

    Assimp::Importer importer;

    // Scale
    if (opts.scale != 1.0f)
        importer.SetPropertyFloat(AI_CONFIG_GLOBAL_SCALE_FACTOR_KEY, opts.scale);

    unsigned int flags =
        aiProcess_Triangulate            |
        aiProcess_JoinIdenticalVertices  |
        aiProcess_SortByPType            |
        aiProcess_GlobalScale;

    if (opts.generateNormals)
        flags |= aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace;
    if (opts.flipUVs)
        flags |= aiProcess_FlipUVs;

    const aiScene* aisc = importer.ReadFile(loadPath, flags);
    if (!aisc || (aisc->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !aisc->mRootNode) {
        TraceLog(LOG_ERROR, "SceneImporter: Assimp error: %s", importer.GetErrorString());
        return nullptr;
    }

    auto scene = std::make_unique<ImportedScene>();
    scene->path = loadPath;

    // Base path for texture resolution
    std::string basePath;
    size_t sep = loadPath.find_last_of("/\\");
    if (sep != std::string::npos) basePath = loadPath.substr(0, sep);

    // ── Lights ───────────────────────────────────────────────────────────────
    for (unsigned int i = 0; i < aisc->mNumLights; ++i)
        scene->lights.push_back(ExtractLight(aisc->mLights[i], aisc));

    // ── Node tree + meshes ────────────────────────────────────────────────────
    aiMatrix4x4 identity;
    BuildContext ctx{ aisc, scene.get(), basePath, opts, {} };
    int rootIdx = WalkNode(aisc->mRootNode, -1, identity, ctx);
    scene->rootNodes.push_back(rootIdx);

    TraceLog(LOG_INFO, "SceneImporter: loaded '%s' — %d meshes, %d nodes, %d lights",
             loadPath.c_str(),
             (int)scene->meshes.size(),
             (int)scene->nodes.size(),
             (int)scene->lights.size());

    return scene;
}

} // namespace Hotones
