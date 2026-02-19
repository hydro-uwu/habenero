#pragma once

struct lua_State;

namespace Hotones::Scripting::LuaLoader {

// Register the `audio` table into the Lua state.
//
// audio.loadSound(name, path)            -- load a file under a logical name
// audio.play(name [, gain])              -- play first loaded variant
// audio.playRandom(name [, gain])        -- play a random variant
// audio.playSequential(name [, gain])    -- round-robin through variants
// audio.setVolume(vol)                   -- master volume 0-100
// audio.getVolume()          -> number   -- current master volume
// audio.stopAll()                        -- stop all playing sounds
void registerAudio(lua_State* L);

} // namespace Hotones::Scripting::LuaLoader
