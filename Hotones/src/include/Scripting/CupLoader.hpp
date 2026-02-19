#pragma once

#include <cstdint>
#include <string>

struct lua_State;

namespace Hotones::Scripting {

class CupPackage;

class CupLoader {
public:
    CupLoader();
    ~CupLoader();

    // Initialize Lua state and register ALL engine libraries
    // (render, audio, input, server).
    bool init();

    // Load a Lua file onto the stack (does not execute it yet).
    bool loadScript(const std::string& path);

    // Execute the previously loaded chunk.
    bool run();

    // ── Cup pack API ─────────────────────────────────────────────────────────

    // Load, execute, and initialise a CupPackage:
    //   1. Executes init.lua found in the pack root.
    //   2. Reads the global Init table (MainScene, MainClass, Debug).
    //   3. Stores a reference to Init.MainClass.
    //   4. Calls MainClass:Init().
    // init() must be called before loadPak().
    bool loadPak(CupPackage& pkg);

    // Call MainClass:Update() — drive from the server / game tick loop.
    void update();

    // Call MainClass:draw3D() — 3D pass; called INSIDE BeginMode3D() / EndMode3D().
    // Use mesh.* Lua functions here (they call raylib 3D primitives directly).
    void draw3D();

    // Call MainClass:Draw() — 2D overlay pass; called AFTER EndMode3D() (client only).
    void draw();

    // ── Player event hooks ────────────────────────────────────────────────────
    // Call MainClass:onPlayerJoined(id, name) if the method exists.
    void firePlayerJoined(uint8_t id, const char* name);
    // Call MainClass:onPlayerLeft(id) if the method exists.
    void firePlayerLeft(uint8_t id);

    // Path declared in Init.MainScene, resolved to an absolute path.
    // Empty string if none was declared or loadPak has not been called.
    const std::string& mainScenePath() const { return m_mainScene; }

    // Access the raw Lua state for advanced usage.
    lua_State* state();

private:
    // Push instance + method, call with args already on stack, handle errors.
    // nargs = number of extra arguments above the implicit `self`.
    bool callMethod(const char* method, int nargs = 0);

    lua_State*  L;
    std::string m_mainScene;
    int         m_classRef;  ///< LUA_REGISTRY key of MainClass table; LUA_NOREF = none
};

} // namespace Hotones::Scripting
