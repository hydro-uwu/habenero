#pragma once

#include <string>

struct lua_State;

namespace Hotones::Scripting {

class CupLoader {
public:
    CupLoader();
    ~CupLoader();

    // Initialize Lua state and register libraries
    bool init();

    // Load a Lua file (does not execute it yet)
    bool loadScript(const std::string &path);

    // Execute the previously loaded chunk
    bool run();

    // Access raw state for advanced usage
    lua_State* state();

private:
    lua_State* L;
};

} // namespace Hotones::Scripting
