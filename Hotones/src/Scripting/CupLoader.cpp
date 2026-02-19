// Minimal Lua runtime integration for the project.
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <raylib.h>
#include "../include/Scripting/CupLoader.hpp"
#include "../include/Scripting/CupPackage.hpp"
#include "../include/Scripting/LuaLoader/Rendering.hpp"
#include "../include/Scripting/LuaLoader/Audio.hpp"
#include "../include/Scripting/LuaLoader/Input.hpp"
#include "../include/Scripting/LuaLoader/ServerLib.hpp"
#include "../include/Scripting/LuaLoader/MeshGen.hpp"

#include <lua.hpp>

// ── Timing globals (work in both headless and windowed Lua contexts) ──────────
namespace {
    static std::chrono::steady_clock::time_point g_luaLastFrame;
    static std::chrono::steady_clock::time_point g_luaStartTime;
    static bool g_luaTimingInit = false;

    // GetFrameTime() — seconds elapsed since the last call (or 0 on first call)
    static int l_GetFrameTime(lua_State* L) {
        auto now = std::chrono::steady_clock::now();
        float dt = 0.0f;
        if (g_luaTimingInit) {
            dt = std::chrono::duration<float>(now - g_luaLastFrame).count();
        } else {
            g_luaStartTime  = now;
            g_luaTimingInit = true;
        }
        g_luaLastFrame = now;
        lua_pushnumber(L, (lua_Number)dt);
        return 1;
    }

    // GetTime() — seconds since the Lua state was initialised
    static int l_GetTime(lua_State* L) {
        if (!g_luaTimingInit) {
            lua_pushnumber(L, 0.0);
            return 1;
        }
        auto now = std::chrono::steady_clock::now();
        double secs = std::chrono::duration<double>(now - g_luaStartTime).count();
        lua_pushnumber(L, (lua_Number)secs);
        return 1;
    }
} // anonymous namespace

namespace Hotones::Scripting {

CupLoader::CupLoader()
    : L(nullptr), m_classRef(LUA_NOREF)
{
}

CupLoader::~CupLoader()
{
    if (L) {
        if (m_classRef != LUA_NOREF)
            luaL_unref(L, LUA_REGISTRYINDEX, m_classRef);
        lua_close(L);
    }
}

bool CupLoader::init()
{
    L = luaL_newstate();
    if (!L) return false;
    luaL_openlibs(L);

    Hotones::Scripting::LuaLoader::registerRendering(L);
    Hotones::Scripting::LuaLoader::registerAudio(L);
    Hotones::Scripting::LuaLoader::registerInput(L);
    Hotones::Scripting::LuaLoader::registerServer(L);
    Hotones::Scripting::LuaLoader::registerMeshGen(L);

    // Register timing globals so Lua scripts work in both headless and windowed modes
    g_luaTimingInit = false; // reset on each new Lua state
    lua_pushcfunction(L, l_GetFrameTime);
    lua_setglobal(L, "GetFrameTime");
    lua_pushcfunction(L, l_GetTime);
    lua_setglobal(L, "GetTime");

    return true;
}

bool CupLoader::loadScript(const std::string& path)
{
    if (!L) return false;
    if (!std::filesystem::exists(path)) {
        TraceLog(LOG_ERROR, "[CupLoader] Script not found: %s", path.c_str());
        return false;
    }
    int status = luaL_loadfile(L, path.c_str());
    if (status != LUA_OK) {
        const char* msg = lua_tostring(L, -1);
        TraceLog(LOG_ERROR, "[CupLoader] Load error: %s", (msg ? msg : "<unknown>"));
        lua_pop(L, 1);
        return false;
    }
    return true;
}

bool CupLoader::run()
{
    if (!L) return false;
    int status = lua_pcall(L, 0, LUA_MULTRET, 0);
    if (status != LUA_OK) {
        const char* msg = lua_tostring(L, -1);
        TraceLog(LOG_ERROR, "[CupLoader] Runtime error: %s", (msg ? msg : "<unknown>"));
        lua_pop(L, 1);
        return false;
    }
    return true;
}

bool CupLoader::loadPak(CupPackage& pkg)
{
    if (!L) {
        TraceLog(LOG_ERROR, "[CupLoader] Call init() before loadPak().");
        return false;
    }
    if (!pkg.isOpen()) {
        TraceLog(LOG_ERROR, "[CupLoader] CupPackage is not open.");
        return false;
    }

    const std::string initPath = pkg.initScript();
    if (!std::filesystem::exists(initPath)) {
        TraceLog(LOG_ERROR, "[CupLoader] Pack is missing init.lua: %s", initPath.c_str());
        return false;
    }
    if (!loadScript(initPath) || !run())
        return false;

    lua_getglobal(L, "Init");
    if (!lua_istable(L, -1)) {
        TraceLog(LOG_ERROR, "[CupLoader] init.lua must declare a global 'Init' table.");
        lua_pop(L, 1);
        return false;
    }

    lua_getfield(L, -1, "MainScene");
    if (lua_isstring(L, -1)) {
        const char* rel = lua_tostring(L, -1);
        m_mainScene = (std::filesystem::path(pkg.rootPath()) / rel).string();
    }
    lua_pop(L, 1);

    lua_getfield(L, -1, "Debug");
    if (lua_toboolean(L, -1))
        TraceLog(LOG_INFO, "[CupLoader] Pack debug mode enabled.");
    lua_pop(L, 1);

    lua_getfield(L, -1, "MainClass");
    if (lua_istable(L, -1)) {
        m_classRef = luaL_ref(L, LUA_REGISTRYINDEX);
        } else {
        lua_pop(L, 1);
        TraceLog(LOG_WARNING, "[CupLoader] Warning: Init.MainClass is not a table.");
    }
    lua_pop(L, 1);

    callMethod("Init");
    return true;
}

void CupLoader::update()  { callMethod("Update");  }
void CupLoader::draw3D()  { callMethod("draw3D");  }
void CupLoader::draw()    { callMethod("Draw");    }

void CupLoader::firePlayerJoined(uint8_t id, const char* name)
{
    if (!L || m_classRef == LUA_NOREF) return;
    lua_rawgeti(L, LUA_REGISTRYINDEX, m_classRef);
    lua_getfield(L, -1, "onPlayerJoined");
    if (!lua_isfunction(L, -1)) { lua_pop(L, 2); return; }
    lua_pushvalue(L, -2);
    lua_pushinteger(L, id);
    lua_pushstring(L, name ? name : "");
    if (lua_pcall(L, 3, 0, 0) != LUA_OK) {
        const char* err = lua_tostring(L, -1);
        TraceLog(LOG_ERROR, "[CupLoader] onPlayerJoined() error: %s", (err ? err : "<unknown>"));
        lua_pop(L, 1);
    }
    lua_pop(L, 1);
}

void CupLoader::firePlayerLeft(uint8_t id)
{
    if (!L || m_classRef == LUA_NOREF) return;
    lua_rawgeti(L, LUA_REGISTRYINDEX, m_classRef);
    lua_getfield(L, -1, "onPlayerLeft");
    if (!lua_isfunction(L, -1)) { lua_pop(L, 2); return; }
    lua_pushvalue(L, -2);
    lua_pushinteger(L, id);
    if (lua_pcall(L, 2, 0, 0) != LUA_OK) {
        const char* err = lua_tostring(L, -1);
        TraceLog(LOG_ERROR, "[CupLoader] onPlayerLeft() error: %s", (err ? err : "<unknown>"));
        lua_pop(L, 1);
    }
    lua_pop(L, 1);
}

bool CupLoader::callMethod(const char* method, int /*nargs*/)
{
    if (!L || m_classRef == LUA_NOREF) return false;

    lua_rawgeti(L, LUA_REGISTRYINDEX, m_classRef);
    if (!lua_istable(L, -1)) { lua_pop(L, 1); return false; }

    lua_getfield(L, -1, method);
    if (!lua_isfunction(L, -1)) { lua_pop(L, 2); return false; }

    lua_pushvalue(L, -2);
    if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
        const char* err = lua_tostring(L, -1);
        TraceLog(LOG_ERROR, "[CupLoader] %s() error: %s", method, (err ? err : "<unknown>"));
        lua_pop(L, 2);
        return false;
    }

    lua_pop(L, 1);
    return true;
}

lua_State* CupLoader::state()
{
    return L;
}

} // namespace Hotones::Scripting
