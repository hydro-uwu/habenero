#include <raylib.h>
#include <lua.hpp>
#include "../../include/Scripting/LuaLoader/MeshGen.hpp"

// All functions here call raylib 3D primitives directly.
// They must only be called from Lua's draw3D() callback, which the engine
// invokes between BeginMode3D() / EndMode3D().

namespace Hotones::Scripting::LuaLoader {

// Helper: read an optional RGBA block starting at stack index `first`.
static Color optColor(lua_State* L, int first,
                      unsigned char dr = 255, unsigned char dg = 255,
                      unsigned char db = 255, unsigned char da = 255)
{
    Color c;
    c.r = (unsigned char)luaL_optinteger(L, first,     dr);
    c.g = (unsigned char)luaL_optinteger(L, first + 1, dg);
    c.b = (unsigned char)luaL_optinteger(L, first + 2, db);
    c.a = (unsigned char)luaL_optinteger(L, first + 3, da);
    return c;
}

// mesh.plane(x, y, z, width, depth [, r, g, b, a])
static int l_plane(lua_State* L)
{
    float x = (float)luaL_checknumber(L, 1);
    float y = (float)luaL_checknumber(L, 2);
    float z = (float)luaL_checknumber(L, 3);
    float w = (float)luaL_checknumber(L, 4);
    float d = (float)luaL_checknumber(L, 5);
    Color c = optColor(L, 6, 100, 180, 100);
    DrawPlane({ x, y, z }, { w, d }, c);
    return 0;
}

// mesh.box(x, y, z, width, height, depth [, r, g, b, a])
static int l_box(lua_State* L)
{
    float x = (float)luaL_checknumber(L, 1);
    float y = (float)luaL_checknumber(L, 2);
    float z = (float)luaL_checknumber(L, 3);
    float w = (float)luaL_checknumber(L, 4);
    float h = (float)luaL_checknumber(L, 5);
    float d = (float)luaL_checknumber(L, 6);
    Color c = optColor(L, 7);
    DrawCubeV({ x, y, z }, { w, h, d }, c);
    return 0;
}

// mesh.boxWires(x, y, z, width, height, depth [, r, g, b, a])
static int l_boxWires(lua_State* L)
{
    float x = (float)luaL_checknumber(L, 1);
    float y = (float)luaL_checknumber(L, 2);
    float z = (float)luaL_checknumber(L, 3);
    float w = (float)luaL_checknumber(L, 4);
    float h = (float)luaL_checknumber(L, 5);
    float d = (float)luaL_checknumber(L, 6);
    Color c = optColor(L, 7, 200, 200, 200);
    DrawCubeWiresV({ x, y, z }, { w, h, d }, c);
    return 0;
}

// mesh.sphere(x, y, z, radius [, rings, slices, r, g, b, a])
static int l_sphere(lua_State* L)
{
    float x      = (float)luaL_checknumber(L, 1);
    float y      = (float)luaL_checknumber(L, 2);
    float z      = (float)luaL_checknumber(L, 3);
    float radius = (float)luaL_checknumber(L, 4);
    int   rings  = (int)luaL_optinteger(L, 5, 16);
    int   slices = (int)luaL_optinteger(L, 6, 16);
    Color c      = optColor(L, 7);
    DrawSphereEx({ x, y, z }, radius, rings, slices, c);
    return 0;
}

// mesh.cylinder(x, y, z, radiusTop, radiusBottom, height [, slices, r, g, b, a])
static int l_cylinder(lua_State* L)
{
    float x    = (float)luaL_checknumber(L, 1);
    float y    = (float)luaL_checknumber(L, 2);
    float z    = (float)luaL_checknumber(L, 3);
    float rtop = (float)luaL_checknumber(L, 4);
    float rbot = (float)luaL_checknumber(L, 5);
    float h    = (float)luaL_checknumber(L, 6);
    int   slic = (int)luaL_optinteger(L, 7, 16);
    Color c    = optColor(L, 8);
    DrawCylinderEx({ x, y, z }, { x, y + h, z }, rbot, rtop, slic, c);
    return 0;
}

// mesh.line(x1, y1, z1, x2, y2, z2 [, r, g, b, a])
static int l_line(lua_State* L)
{
    Vector3 a = { (float)luaL_checknumber(L,1),
                  (float)luaL_checknumber(L,2),
                  (float)luaL_checknumber(L,3) };
    Vector3 b = { (float)luaL_checknumber(L,4),
                  (float)luaL_checknumber(L,5),
                  (float)luaL_checknumber(L,6) };
    Color c   = optColor(L, 7);
    DrawLine3D(a, b, c);
    return 0;
}

// mesh.grid(slices, spacing)
static int l_grid(lua_State* L)
{
    int   slices  = (int)luaL_optinteger(L, 1, 20);
    float spacing = (float)luaL_optnumber(L,  2, 1.0);
    DrawGrid(slices, spacing);
    return 0;
}

// mesh.axes(x, y, z, size)
static int l_axes(lua_State* L)
{
    float x    = (float)luaL_optnumber(L, 1, 0.0);
    float y    = (float)luaL_optnumber(L, 2, 0.0);
    float z    = (float)luaL_optnumber(L, 3, 0.0);
    float size = (float)luaL_optnumber(L, 4, 1.0);
    // X — red
    DrawLine3D({ x, y, z }, { x + size, y, z }, RED);
    // Y — green
    DrawLine3D({ x, y, z }, { x, y + size, z }, GREEN);
    // Z — blue
    DrawLine3D({ x, y, z }, { x, y, z + size }, BLUE);
    return 0;
}

void registerMeshGen(lua_State* L)
{
    static const luaL_Reg funcs[] = {
        {"plane",    l_plane},
        {"box",      l_box},
        {"boxWires", l_boxWires},
        {"sphere",   l_sphere},
        {"cylinder", l_cylinder},
        {"line",     l_line},
        {"grid",     l_grid},
        {"axes",     l_axes},
        {nullptr, nullptr}
    };

    luaL_newlib(L, funcs);
    lua_setglobal(L, "mesh");
}

} // namespace Hotones::Scripting::LuaLoader
