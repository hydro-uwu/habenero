#pragma once

struct lua_State;

namespace Hotones::Scripting::LuaLoader {

// Register the `server` table into the Lua state.
//
// server.log(msg)       -- print to stdout with a [Script] prefix
// server.getTime()      -> number  -- seconds elapsed since the server started
void registerServer(lua_State* L);

} // namespace Hotones::Scripting::LuaLoader
