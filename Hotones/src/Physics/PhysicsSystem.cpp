// Physics backend: triangle-accurate sphere sweeps via a mid-phase BVH.
//
// Design:
//   BuildBVH()              — recursive median-split BVH over triangles
//   SweepSphereNode()       — traverse BVH, run analytic sphere-vs-tri test per leaf
//   PenetrationSphereNode() — traverse BVH, resolve sphere-vs-tri overlap
//
// Sphere-vs-triangle sweep:
//   We cast a ray from (start) to (end) against the "inflated" geometry of each
//   triangle (the Minkowski sum of the triangle with a sphere of the given radius).
//   That Minkowski sum consists of:
//     1. The triangle face (ray vs plane, clamped to triangle)
//     2. Three edge capsules (ray vs infinite cylinder for each edge)
//     3. Three vertex spheres
//   We return the earliest parametric hit t ∈ [0,1].

#include "../include/Physics/PhysicsSystem.hpp"
#include <algorithm>
#include <cfloat>
#include <cstring>
#include <iostream>
#include <memory>
#include <mutex>
#include <vector>
#include <raymath.h>

// ─── Geometry helpers (file-internal) ────────────────────────────────────────

static inline float v3dot(Vector3 a, Vector3 b) { return Vector3DotProduct(a, b); }
static inline float v3len(Vector3 a)             { return Vector3Length(a); }
static inline Vector3 v3norm(Vector3 a)           { return Vector3Normalize(a); }
static inline Vector3 v3sub(Vector3 a, Vector3 b) { return Vector3Subtract(a, b); }
static inline Vector3 v3add(Vector3 a, Vector3 b) { return Vector3Add(a, b); }
static inline Vector3 v3scale(Vector3 a, float s) { return Vector3Scale(a, s); }
static inline Vector3 v3cross(Vector3 a, Vector3 b){ return Vector3CrossProduct(a, b); }

// Closest point on triangle (abc) to point p — Ericson §5.1.5
static Vector3 ClosestPtTriangle(Vector3 p, Vector3 a, Vector3 b, Vector3 c) {
    Vector3 ab = v3sub(b,a), ac = v3sub(c,a), ap = v3sub(p,a);
    float d1 = v3dot(ab,ap), d2 = v3dot(ac,ap);
    if (d1 <= 0.f && d2 <= 0.f) return a;

    Vector3 bp = v3sub(p,b);
    float d3 = v3dot(ab,bp), d4 = v3dot(ac,bp);
    if (d3 >= 0.f && d4 <= d3) return b;

    float vc = d1*d4 - d3*d2;
    if (vc <= 0.f && d1 >= 0.f && d3 <= 0.f) {
        float v = d1 / (d1 - d3);
        return v3add(a, v3scale(ab, v));
    }

    Vector3 cp = v3sub(p,c);
    float d5 = v3dot(ab,cp), d6 = v3dot(ac,cp);
    if (d6 >= 0.f && d5 <= d6) return c;

    float vb = d5*d2 - d1*d6;
    if (vb <= 0.f && d2 >= 0.f && d6 <= 0.f) {
        float w = d2 / (d2 - d6);
        return v3add(a, v3scale(ac, w));
    }

    float va = d3*d6 - d5*d4;
    float denom = d4 - d3 + d5 - d6;
    if (va <= 0.f && denom > 0.f) {
        float w = (d4 - d3) / denom;
        return v3add(b, v3scale(v3sub(c,b), w));
    }

    float dv = 1.f / (va + vb + vc);
    float vv = vb * dv, wv = vc * dv;
    return v3add(a, v3add(v3scale(ab,vv), v3scale(ac,wv)));
}

// Analytic ray-vs-sphere: ray o+t*d, sphere center c radius r.
// Returns t of first intersection, or FLT_MAX if none in [tMin,tMax].
static float RaySphere(Vector3 o, Vector3 d, Vector3 c, float r, float tMin, float tMax) {
    Vector3 oc = v3sub(o, c);
    float A = v3dot(d,d);
    float B = 2.f * v3dot(oc, d);
    float C = v3dot(oc, oc) - r*r;
    float disc = B*B - 4.f*A*C;
    if (disc < 0.f) return FLT_MAX;
    float sqrtD = sqrtf(disc);
    float t = (-B - sqrtD) / (2.f*A);
    if (t >= tMin && t <= tMax) return t;
    t = (-B + sqrtD) / (2.f*A);
    if (t >= tMin && t <= tMax) return t;
    return FLT_MAX;
}

// Analytic ray-vs-infinite-cylinder (axis a→b, radius r).
// Returns t of first lateral intersection, or FLT_MAX if none.
static float RayCylinder(Vector3 ro, Vector3 rd, Vector3 a, Vector3 b, float r, float tMin, float tMax) {
    Vector3 ab  = v3sub(b, a);
    Vector3 ao  = v3sub(ro, a);
    float abLen2 = v3dot(ab, ab);
    if (abLen2 < 1e-10f) return FLT_MAX;

    // Project rd and ao onto plane perpendicular to ab
    float rdDotAb = v3dot(rd, ab) / abLen2;
    float aoDotAb = v3dot(ao, ab) / abLen2;
    Vector3 d_perp = v3sub(rd, v3scale(ab, rdDotAb));
    Vector3 o_perp = v3sub(ao, v3scale(ab, aoDotAb));

    float A = v3dot(d_perp, d_perp);
    float B = 2.f * v3dot(o_perp, d_perp);
    float C = v3dot(o_perp, o_perp) - r*r;
    float disc = B*B - 4.f*A*C;
    if (disc < 0.f || A < 1e-10f) return FLT_MAX;
    float sqrtD = sqrtf(disc);
    float t = (-B - sqrtD) / (2.f*A);
    if (t < tMin || t > tMax) {
        t = (-B + sqrtD) / (2.f*A);
        if (t < tMin || t > tMax) return FLT_MAX;
    }
    // Check that the hit lies between a and b along the cylinder axis
    Vector3 hitPt = v3add(ro, v3scale(rd, t));
    float proj = v3dot(v3sub(hitPt, a), ab) / abLen2;
    if (proj < 0.f || proj > 1.f) return FLT_MAX;
    return t;
}

// Continuous sphere vs triangle sweep.
// Returns t ∈ [0, segLen/segLen=1] of first contact, FLT_MAX if no hit.
// outNormal filled with the contact normal at impact.
static float SweepSphereTriangle(Vector3 start, Vector3 end, float radius,
                                  Vector3 ta, Vector3 tb, Vector3 tc,
                                  Vector3& outNormal) {
    Vector3 d    = v3sub(end, start);
    float segLen = v3len(d);
    if (segLen < 1e-10f) return FLT_MAX;

    Vector3 triNorm = v3norm(v3cross(v3sub(tb,ta), v3sub(tc,ta)));
    float bestT = FLT_MAX;
    Vector3 bestN = triNorm;

    // ── 1. Ray vs face (inflated by radius along normal) ─────────────────────
    {
        float nDotD = v3dot(triNorm, d);
        if (fabsf(nDotD) > 1e-8f) {
            // Inflate plane by radius toward sphere origin
            for (int sign = -1; sign <= 1; sign += 2) {
                Vector3 planePoint = v3add(ta, v3scale(triNorm, sign * radius));
                float nDotOs = v3dot(triNorm, v3sub(planePoint, start));
                float t = nDotOs / nDotD;
                if (t >= 0.f && t < bestT) {
                    // Check if hit point (back-projected onto triangle plane) is inside triangle
                    Vector3 hitPt    = v3add(start, v3scale(d, t));
                    Vector3 onPlane  = v3sub(hitPt, v3scale(triNorm, sign * radius));
                    Vector3 closest  = ClosestPtTriangle(onPlane, ta, tb, tc);
                    if (v3len(v3sub(onPlane, closest)) < 1e-4f) {
                        bestT = t;
                        bestN = v3scale(triNorm, (float)sign);
                    }
                }
            }
        }
    }

    // ── 2. Ray vs edge capsules ───────────────────────────────────────────────
    Vector3 edges[3][2] = { {ta,tb}, {tb,tc}, {tc,ta} };
    for (auto& e : edges) {
        float t = RayCylinder(start, d, e[0], e[1], radius, 0.f, bestT);
        if (t < bestT) {
            // Compute normal = (hitPoint - closestPointOnEdge) normalised
            Vector3 hitPt   = v3add(start, v3scale(d, t));
            Vector3 ab      = v3sub(e[1], e[0]);
            float abL2      = v3dot(ab,ab);
            float proj      = abL2 > 1e-10f ? v3dot(v3sub(hitPt, e[0]), ab) / abL2 : 0.f;
            proj             = proj < 0.f ? 0.f : (proj > 1.f ? 1.f : proj);
            Vector3 closest = v3add(e[0], v3scale(ab, proj));
            Vector3 n       = v3sub(hitPt, closest);
            float nlen      = v3len(n);
            if (nlen > 1e-6f) {
                bestT = t;
                bestN = v3scale(n, 1.f/nlen);
            }
        }
    }

    // ── 3. Ray vs vertex spheres ──────────────────────────────────────────────
    Vector3 verts[3] = { ta, tb, tc };
    for (auto& v : verts) {
        float t = RaySphere(start, d, v, radius, 0.f, bestT);
        if (t < bestT) {
            Vector3 hitPt = v3add(start, v3scale(d, t));
            Vector3 n     = v3sub(hitPt, v);
            float nlen    = v3len(n);
            if (nlen > 1e-6f) {
                bestT = t;
                bestN = v3scale(n, 1.f/nlen);
            }
        }
    }

    if (bestT > 1.f + 1e-6f) return FLT_MAX; // no hit within segment
    outNormal = bestN;
    return bestT;
}

// ─── BVH ─────────────────────────────────────────────────────────────────────

struct Tri {
    Vector3 a, b, c;
    Vector3 centroid;
};

struct BVHNode {
    // AABB enclosing all triangles in this subtree
    Vector3 bmin, bmax;
    // If leaf: [triStart, triStart+triCount)  in the reordered triangle array
    // If internal: left child = index+1, right child = rightChild
    int triStart = 0, triCount = 0;
    int rightChild = -1; // -1 → leaf
};

struct BVH {
    std::vector<BVHNode> nodes;
    std::vector<Tri>     tris;   // reordered

    // Build from a flat triangle list
    void Build(std::vector<Tri>&& inTris) {
        tris = std::move(inTris);
        if (tris.empty()) return;
        nodes.clear();
        nodes.reserve(tris.size() * 2);
        BuildNode(0, (int)tris.size(), 0);
    }

private:
    static Vector3 TriAabbMin(const Tri& t) {
        return { fminf(t.a.x, fminf(t.b.x, t.c.x)),
                 fminf(t.a.y, fminf(t.b.y, t.c.y)),
                 fminf(t.a.z, fminf(t.b.z, t.c.z)) };
    }
    static Vector3 TriAabbMax(const Tri& t) {
        return { fmaxf(t.a.x, fmaxf(t.b.x, t.c.x)),
                 fmaxf(t.a.y, fmaxf(t.b.y, t.c.y)),
                 fmaxf(t.a.z, fmaxf(t.b.z, t.c.z)) };
    }

    int BuildNode(int start, int end, int /*depth*/) {
        int nodeIdx = (int)nodes.size();
        nodes.push_back({});
        BVHNode& node = nodes[nodeIdx];

        // Compute AABB
        node.bmin = TriAabbMin(tris[start]);
        node.bmax = TriAabbMax(tris[start]);
        for (int i = start+1; i < end; ++i) {
            Vector3 mn = TriAabbMin(tris[i]);
            Vector3 mx = TriAabbMax(tris[i]);
            node.bmin = { fminf(node.bmin.x, mn.x), fminf(node.bmin.y, mn.y), fminf(node.bmin.z, mn.z) };
            node.bmax = { fmaxf(node.bmax.x, mx.x), fmaxf(node.bmax.y, mx.y), fmaxf(node.bmax.z, mx.z) };
        }

        int count = end - start;
        if (count <= 4) {
            // Leaf
            node.triStart = start;
            node.triCount = count;
            node.rightChild = -1;
            return nodeIdx;
        }

        // Split on longest axis at centroid median
        Vector3 ext = v3sub(node.bmax, node.bmin);
        int axis = (ext.x > ext.y && ext.x > ext.z) ? 0 : (ext.y > ext.z ? 1 : 2);
        float mid = 0.f;
        for (int i = start; i < end; ++i) {
            float* c = &tris[i].centroid.x;
            mid += c[axis];
        }
        mid /= (float)count;

        auto midIt = std::partition(tris.begin() + start, tris.begin() + end,
                                    [axis, mid](const Tri& t){
                                        const float* c = &t.centroid.x;
                                        return c[axis] < mid;
                                    });
        int split = (int)(midIt - tris.begin());
        if (split == start || split == end) split = start + count / 2;

        node.triStart = -1; node.triCount = 0;
        BuildNode(start, split, 0);                    // left child (always nodeIdx+1)
        node.rightChild = BuildNode(split, end, 0);    // right child
        // Re-fetch reference after possible vector reallocation
        nodes[nodeIdx].rightChild = node.rightChild;
        return nodeIdx;
    }
};

// AABB vs expanded AABB (expand box by radius) overlap check
static bool AabbOverlap(Vector3 bmin, Vector3 bmax, Vector3 qmin, Vector3 qmax) {
    return (bmin.x <= qmax.x && bmax.x >= qmin.x) &&
           (bmin.y <= qmax.y && bmax.y >= qmin.y) &&
           (bmin.z <= qmax.z && bmax.z >= qmin.z);
}

// Traverse BVH for sweep; returns earliest t.
static void SweepNodeBVH(const BVH& bvh, int nodeIdx,
                          Vector3 start, Vector3 end, float radius,
                          float& bestT, Vector3& bestN) {
    if (nodeIdx < 0 || nodeIdx >= (int)bvh.nodes.size()) return;
    const BVHNode& node = bvh.nodes[nodeIdx];

    // Expand node AABB by radius and do a quick overlap test with the swept AABB of the sphere
    Vector3 swMin = { fminf(start.x, end.x) - radius,
                      fminf(start.y, end.y) - radius,
                      fminf(start.z, end.z) - radius };
    Vector3 swMax = { fmaxf(start.x, end.x) + radius,
                      fmaxf(start.y, end.y) + radius,
                      fmaxf(start.z, end.z) + radius };
    if (!AabbOverlap(node.bmin, node.bmax, swMin, swMax)) return;

    if (node.rightChild == -1) {
        // Leaf — test each triangle
        for (int i = node.triStart; i < node.triStart + node.triCount; ++i) {
            const Tri& tri = bvh.tris[i];
            Vector3 n;
            float t = SweepSphereTriangle(start, end, radius, tri.a, tri.b, tri.c, n);
            if (t < bestT) { bestT = t; bestN = n; }
        }
        return;
    }
    // Internal — recurse both children
    SweepNodeBVH(bvh, nodeIdx + 1,        start, end, radius, bestT, bestN);
    SweepNodeBVH(bvh, node.rightChild,    start, end, radius, bestT, bestN);
}

// Traverse BVH for penetration resolution — collect all triangles whose closest
// point to `center` is within `radius`.
static void PenetrationNodeBVH(const BVH& bvh, int nodeIdx,
                                Vector3 center, float radius,
                                Vector3& outPush, bool& didPush) {
    if (nodeIdx < 0 || nodeIdx >= (int)bvh.nodes.size()) return;
    const BVHNode& node = bvh.nodes[nodeIdx];

    // Quick AABB cull (expand by radius)
    if (center.x + radius < node.bmin.x || center.x - radius > node.bmax.x ||
        center.y + radius < node.bmin.y || center.y - radius > node.bmax.y ||
        center.z + radius < node.bmin.z || center.z - radius > node.bmax.z) return;

    if (node.rightChild == -1) {
        for (int i = node.triStart; i < node.triStart + node.triCount; ++i) {
            const Tri& tri = bvh.tris[i];
            Vector3 closest = ClosestPtTriangle(center, tri.a, tri.b, tri.c);
            Vector3 diff    = v3sub(center, closest);
            float dist2     = v3dot(diff, diff);
            if (dist2 < radius * radius) {
                float dist = sqrtf(dist2);
                Vector3 n;
                if (dist > 1e-6f) {
                    n = v3scale(diff, 1.f / dist);
                } else {
                    // Center is on the triangle — push out along face normal
                    n = v3norm(v3cross(v3sub(tri.b, tri.a), v3sub(tri.c, tri.a)));
                }
                float depth = radius - dist;
                outPush  = v3add(outPush, v3scale(n, depth));
                didPush  = true;
            }
        }
        return;
    }
    PenetrationNodeBVH(bvh, nodeIdx + 1,     center, radius, outPush, didPush);
    PenetrationNodeBVH(bvh, node.rightChild, center, radius, outPush, didPush);
}

// ─── Static mesh registry ─────────────────────────────────────────────────────

struct StaticMeshEntry {
    int  handle = 0;
    BVH  bvh;
};

static std::vector<StaticMeshEntry> g_staticMeshes;
static int                          g_nextHandle = 1;
static std::mutex                   g_meshMutex;

namespace Hotones { namespace Physics {

bool InitPhysics()    { return true; }
void ShutdownPhysics(){ g_staticMeshes.clear(); }

int RegisterStaticMeshFromModel(const Model& model, const Vector3& position) {
    if (model.meshCount <= 0 || model.meshes == nullptr) return -1;

    std::vector<Tri> tris;
    tris.reserve(4096);

    for (int mi = 0; mi < model.meshCount; ++mi) {
        const Mesh& m = model.meshes[mi];
        if (m.vertices == nullptr) continue;

        auto addTri = [&](int i0, int i1, int i2) {
            auto vAt = [&](int idx) -> Vector3 {
                return v3add({ m.vertices[idx*3], m.vertices[idx*3+1], m.vertices[idx*3+2] }, position);
            };
            Tri t;
            t.a = vAt(i0); t.b = vAt(i1); t.c = vAt(i2);
            t.centroid = v3scale(v3add(t.a, v3add(t.b, t.c)), 1.f/3.f);
            tris.push_back(t);
        };

        if (m.indices != nullptr) {
            for (int t = 0; t < m.triangleCount; ++t)
                addTri(m.indices[t*3], m.indices[t*3+1], m.indices[t*3+2]);
        } else {
            int triCount = m.vertexCount / 3;
            for (int t = 0; t < triCount; ++t)
                addTri(t*3, t*3+1, t*3+2);
        }
    }

    if (tris.empty()) return -1;

    StaticMeshEntry entry;
    entry.bvh.Build(std::move(tris));

    std::lock_guard<std::mutex> lk(g_meshMutex);
    entry.handle = g_nextHandle++;
    g_staticMeshes.push_back(std::move(entry));
    std::cout << "[Physics] Registered mesh handle=" << g_staticMeshes.back().handle
              << " tris=" << g_staticMeshes.back().bvh.tris.size()
              << " bvh_nodes=" << g_staticMeshes.back().bvh.nodes.size() << "\n";
    return g_staticMeshes.back().handle;
}

void UnregisterStaticMesh(int handle) {
    std::lock_guard<std::mutex> lk(g_meshMutex);
    for (auto it = g_staticMeshes.begin(); it != g_staticMeshes.end(); ++it) {
        if (it->handle == handle) { g_staticMeshes.erase(it); return; }
    }
}

bool SweepSphereAgainstStatic(int handle,
                               const Vector3& start, const Vector3& end,
                               float radius,
                               Vector3& hitPos, Vector3& hitNormal, float& t) {
    // Grab a reference to the entry under lock, then release before traversal
    const BVH* bvhPtr = nullptr;
    {
        std::lock_guard<std::mutex> lk(g_meshMutex);
        for (const auto& e : g_staticMeshes)
            if (e.handle == handle) { bvhPtr = &e.bvh; break; }
        if (!bvhPtr || bvhPtr->nodes.empty()) return false;
    }

    // Safe to read without lock since meshes are immutable once registered
    float bestT = FLT_MAX;
    Vector3 bestN = { 0,1,0 };
    SweepNodeBVH(*bvhPtr, 0, start, end, radius, bestT, bestN);

    if (bestT > 1.f + 1e-6f) return false;

    t         = bestT;
    hitNormal = bestN;
    hitPos    = v3add(start, v3scale(v3sub(end, start), bestT));
    return true;
}

// New: resolve sphere penetration against a registered static mesh.
// Pushes `center` out of all overlapping triangles. Returns true if any push occurred.
bool ResolveSphereAgainstStatic(int handle, Vector3& center, float radius) {
    const BVH* bvhPtr = nullptr;
    {
        std::lock_guard<std::mutex> lk(g_meshMutex);
        for (const auto& e : g_staticMeshes)
            if (e.handle == handle) { bvhPtr = &e.bvh; break; }
        if (!bvhPtr || bvhPtr->nodes.empty()) return false;
    }

    Vector3 totalPush = {0,0,0};
    bool    pushed    = false;
    PenetrationNodeBVH(*bvhPtr, 0, center, radius, totalPush, pushed);
    if (pushed) center = v3add(center, totalPush);
    return pushed;
}

}} // namespace Hotones::Physics
