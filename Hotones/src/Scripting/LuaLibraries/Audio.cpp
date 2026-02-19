#include <lua.hpp>
#include "../../include/Scripting/LuaLoader/Audio.hpp"
#include "../../include/SFX/AudioSystem.hpp"

namespace Hotones::Scripting::LuaLoader {

// audio.loadSound(name, path) -> bool
static int l_loadSound(lua_State* L)
{
    const char* name = luaL_checkstring(L, 1);
    const char* path = luaL_checkstring(L, 2);
    bool ok = Ho_tones::GetSoundBus().LoadSoundFile(name, path);
    lua_pushboolean(L, ok ? 1 : 0);
    return 1;
}

// audio.play(name [, gain]) -> bool
static int l_play(lua_State* L)
{
    const char* name = luaL_checkstring(L, 1);
    float gain = (float)luaL_optnumber(L, 2, 1.0);
    bool ok = Ho_tones::GetSoundBus().PlayLoaded(name, gain);
    lua_pushboolean(L, ok ? 1 : 0);
    return 1;
}

// audio.playRandom(name [, gain]) -> bool
static int l_playRandom(lua_State* L)
{
    const char* name = luaL_checkstring(L, 1);
    float gain = (float)luaL_optnumber(L, 2, 1.0);
    bool ok = Ho_tones::GetSoundBus().PlayRandom(name, gain);
    lua_pushboolean(L, ok ? 1 : 0);
    return 1;
}

// audio.playSequential(name [, gain]) -> bool
static int l_playSequential(lua_State* L)
{
    const char* name = luaL_checkstring(L, 1);
    float gain = (float)luaL_optnumber(L, 2, 1.0);
    bool ok = Ho_tones::GetSoundBus().PlaySequential(name, gain);
    lua_pushboolean(L, ok ? 1 : 0);
    return 1;
}

// audio.setVolume(vol)  -- 0-100
static int l_setVolume(lua_State* L)
{
    int vol = (int)luaL_checkinteger(L, 1);
    Ho_tones::GetSoundBus().SetVolume(vol);
    return 0;
}

// audio.getVolume() -> int
static int l_getVolume(lua_State* L)
{
    lua_pushinteger(L, Ho_tones::GetSoundBus().GetVolume());
    return 1;
}

// audio.stopAll()
static int l_stopAll(lua_State* L)
{
    Ho_tones::GetSoundBus().StopAll();
    return 0;
}

void registerAudio(lua_State* L)
{
    static const luaL_Reg funcs[] = {
        {"loadSound",      l_loadSound},
        {"play",           l_play},
        {"playRandom",     l_playRandom},
        {"playSequential", l_playSequential},
        {"setVolume",      l_setVolume},
        {"getVolume",      l_getVolume},
        {"stopAll",        l_stopAll},
        {nullptr, nullptr}
    };

    luaL_newlib(L, funcs);
    lua_setglobal(L, "audio");
}

} // namespace Hotones::Scripting::LuaLoader
