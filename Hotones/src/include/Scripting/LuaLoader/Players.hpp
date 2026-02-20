#pragma once

struct lua_State;

namespace Hotones::Net { class NetworkManager; }

namespace Hotones::Scripting::LuaLoader {

// Register the `network` table into the Lua state.
//
// network.getPlayers()      -> table[]   -- list of all active remote players
// network.getPlayer(id)     -> table|nil -- single player by ID, or nil
// network.getPlayerCount()  -> integer   -- number of active remote players
// network.getLocalId()      -> integer   -- our own player ID (0 = none)
// network.getMode()         -> string    -- "server" | "client" | "none"
// network.isConnected()     -> boolean   -- true when connected as a client
//
// Each player table contains:
//   id    integer   unique player ID (1-254)
//   name  string    display name
//   x     number    world position X
//   y     number    world position Y
//   z     number    world position Z
//   rotX  number    yaw   (horizontal look angle, radians)
//   rotY  number    pitch (vertical   look angle, radians)
void registerPlayers(lua_State* L, Hotones::Net::NetworkManager* nm = nullptr);

// Update the NetworkManager pointer used by this library at runtime.
// Call whenever the active NetworkManager changes (e.g. after connecting).
void setPlayersNetworkManager(Hotones::Net::NetworkManager* nm);

} // namespace Hotones::Scripting::LuaLoader
