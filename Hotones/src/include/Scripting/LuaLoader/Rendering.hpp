#pragma once

struct lua_State;

namespace Hotones::Scripting::LuaLoader {

// Register rendering-related functions into the given Lua state.
void registerRendering(lua_State* L);

} // namespace Hotones::Scripting::LuaLoader
