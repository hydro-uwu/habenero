#pragma once

struct lua_State;

namespace Hotones::Scripting::LuaLoader {

// Register physics query functions into the given Lua state as the global
// table "physics".
void registerPhysics(lua_State* L);

} // namespace Hotones::Scripting::LuaLoader
