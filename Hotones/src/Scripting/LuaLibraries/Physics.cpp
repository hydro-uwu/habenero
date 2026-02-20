#include <lua.hpp>
#include <raylib.h>
#include "../../include/Scripting/LuaLoader/Physics.hpp"
#include "../../include/Physics/PhysicsSystem.hpp"

namespace Hotones::Scripting::LuaLoader {

// physics.raycast(handle, ox, oy, oz, dx, dy, dz [, maxDist])
//
// Cast a ray from (ox,oy,oz) in direction (dx,dy,dz).
// dir does NOT need to be normalised; the returned t is in the same units
// as the direction vector's length.  maxDist defaults to 1000.
//
// Returns (on hit):   true, hitX, hitY, hitZ, normX, normY, normZ, t
// Returns (on miss):  false
static int l_raycast(lua_State* L) {
    int   handle  = (int)luaL_checkinteger(L, 1);
    float ox      = (float)luaL_checknumber(L, 2);
    float oy      = (float)luaL_checknumber(L, 3);
    float oz      = (float)luaL_checknumber(L, 4);
    float dx      = (float)luaL_checknumber(L, 5);
    float dy      = (float)luaL_checknumber(L, 6);
    float dz      = (float)luaL_checknumber(L, 7);
    float maxDist = (float)luaL_optnumber(L, 8, 1000.0);

    Vector3 origin  = { ox, oy, oz };
    Vector3 dir     = { dx, dy, dz };
    Vector3 hitPos  = { 0, 0, 0 };
    Vector3 hitNorm = { 0, 1, 0 };
    float   t       = 0.f;

    bool hit = Hotones::Physics::RaycastAgainstStatic(
        handle, origin, dir, maxDist, hitPos, hitNorm, t);

    lua_pushboolean(L, hit ? 1 : 0);
    if (hit) {
        lua_pushnumber(L, hitPos.x);
        lua_pushnumber(L, hitPos.y);
        lua_pushnumber(L, hitPos.z);
        lua_pushnumber(L, hitNorm.x);
        lua_pushnumber(L, hitNorm.y);
        lua_pushnumber(L, hitNorm.z);
        lua_pushnumber(L, t);
        return 8;
    }
    return 1;
}

// physics.sweepSphere(handle, sx, sy, sz, ex, ey, ez, radius)
//
// Sweep a sphere of the given radius from (sx,sy,sz) to (ex,ey,ez).
// t ∈ [0,1] is the fraction along the segment where contact occurs.
//
// Returns (on hit):   true, hitX, hitY, hitZ, normX, normY, normZ, t
// Returns (on miss):  false
static int l_sweepSphere(lua_State* L) {
    int   handle = (int)luaL_checkinteger(L, 1);
    float sx     = (float)luaL_checknumber(L, 2);
    float sy     = (float)luaL_checknumber(L, 3);
    float sz     = (float)luaL_checknumber(L, 4);
    float ex     = (float)luaL_checknumber(L, 5);
    float ey     = (float)luaL_checknumber(L, 6);
    float ez     = (float)luaL_checknumber(L, 7);
    float radius = (float)luaL_checknumber(L, 8);

    Vector3 start   = { sx, sy, sz };
    Vector3 end     = { ex, ey, ez };
    Vector3 hitPos  = { 0, 0, 0 };
    Vector3 hitNorm = { 0, 1, 0 };
    float   t       = 0.f;

    bool hit = Hotones::Physics::SweepSphereAgainstStatic(
        handle, start, end, radius, hitPos, hitNorm, t);

    lua_pushboolean(L, hit ? 1 : 0);
    if (hit) {
        lua_pushnumber(L, hitPos.x);
        lua_pushnumber(L, hitPos.y);
        lua_pushnumber(L, hitPos.z);
        lua_pushnumber(L, hitNorm.x);
        lua_pushnumber(L, hitNorm.y);
        lua_pushnumber(L, hitNorm.z);
        lua_pushnumber(L, t);
        return 8;
    }
    return 1;
}

void registerPhysics(lua_State* L) {
    static const luaL_Reg funcs[] = {
        { "raycast",     l_raycast     },
        { "sweepSphere", l_sweepSphere },
        { NULL, NULL }
    };
    luaL_newlib(L, funcs);
    lua_setglobal(L, "physics");
}

} // namespace Hotones::Scripting::LuaLoader
