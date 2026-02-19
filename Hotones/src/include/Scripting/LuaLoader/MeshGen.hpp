#pragma once

struct lua_State;

namespace Hotones::Scripting::LuaLoader {

// Register the `mesh` table into the Lua state.
//
// All mesh.* calls invoke raylib 3D draw functions DIRECTLY.
// They MUST be called from inside your MainClass:draw3D() callback,
// which the engine calls between BeginMode3D() and EndMode3D().
// Calling them from draw() (2D phase) has no effect.
//
// -- Primitives
// mesh.plane(x, y, z, width, depth [, r, g, b, a])
// mesh.box(x, y, z, width, height, depth [, r, g, b, a])
// mesh.boxWires(x, y, z, width, height, depth [, r, g, b, a])
// mesh.sphere(x, y, z, radius [, rings, slices, r, g, b, a])
// mesh.cylinder(x, y, z, radiusTop, radiusBottom, height [, slices, r, g, b, a])
// mesh.line(x1, y1, z1, x2, y2, z2 [, r, g, b, a])
//
// -- Helpers
// mesh.grid(slices, spacing)  -- draws the classic XZ debug grid
// mesh.axes(x, y, z, size)   -- draws RGB axis arrows at position
void registerMeshGen(lua_State* L);

} // namespace Hotones::Scripting::LuaLoader
