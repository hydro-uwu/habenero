#include <chrono>
#include <cstdio>
#include <iostream>
#include <lua.hpp>
#include "../../include/Scripting/LuaLoader/ServerLib.hpp"

namespace Hotones::Scripting::LuaLoader {

namespace {
    // Set once when registerServer() is first called (i.e. when the pack loads).
    static std::chrono::steady_clock::time_point g_startTime;
}

// server.log(msg)
static int l_log(lua_State* L)
{
    const char* msg = luaL_checkstring(L, 1);
    std::cout << "[Script] " << msg << "\n";
    return 0;
}

// server.getTime() -> seconds (float)
static int l_getTime(lua_State* L)
{
    using namespace std::chrono;
    double secs = duration<double>(steady_clock::now() - g_startTime).count();
    lua_pushnumber(L, (lua_Number)secs);
    return 1;
}

void registerServer(lua_State* L)
{
    g_startTime = std::chrono::steady_clock::now();

    static const luaL_Reg funcs[] = {
        {"log",     l_log},
        {"getTime", l_getTime},
        {nullptr, nullptr}
    };

    luaL_newlib(L, funcs);
    lua_setglobal(L, "server");
}

} // namespace Hotones::Scripting::LuaLoader
