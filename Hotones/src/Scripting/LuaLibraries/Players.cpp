#include <lua.hpp>
#include <server/NetworkManager.hpp>
#include "../../include/Scripting/LuaLoader/Players.hpp"

namespace Hotones::Scripting::LuaLoader {

namespace {
    static Net::NetworkManager* g_netMgr = nullptr;

    // Build and push a Lua table for a RemotePlayer snapshot.
    // Leaves the table on top of the stack.
    static void pushPlayerTable(lua_State* L, const Net::RemotePlayer& rp)
    {
        lua_newtable(L);

        lua_pushinteger(L, static_cast<lua_Integer>(rp.id));
        lua_setfield(L, -2, "id");

        lua_pushstring(L, rp.name);
        lua_setfield(L, -2, "name");

        lua_pushnumber(L, static_cast<lua_Number>(rp.posX));
        lua_setfield(L, -2, "x");

        lua_pushnumber(L, static_cast<lua_Number>(rp.posY));
        lua_setfield(L, -2, "y");

        lua_pushnumber(L, static_cast<lua_Number>(rp.posZ));
        lua_setfield(L, -2, "z");

        lua_pushnumber(L, static_cast<lua_Number>(rp.rotX));
        lua_setfield(L, -2, "rotX");

        lua_pushnumber(L, static_cast<lua_Number>(rp.rotY));
        lua_setfield(L, -2, "rotY");
    }
} // anonymous namespace

// ── network.getPlayers() -> table[] ─────────────────────────────────────────
// Returns an array of player tables, one per active remote player.
static int l_getPlayers(lua_State* L)
{
    lua_newtable(L);
    if (!g_netMgr) return 1;

    int idx = 1;
    for (const auto& [id, rp] : g_netMgr->GetRemotePlayers()) {
        if (!rp.active) continue;
        pushPlayerTable(L, rp);
        lua_rawseti(L, -2, idx++);
    }
    return 1;
}

// ── network.getPlayer(id) -> table | nil ─────────────────────────────────────
// Returns a single player table for the given ID, or nil if not found.
static int l_getPlayer(lua_State* L)
{
    lua_Integer id = luaL_checkinteger(L, 1);
    if (!g_netMgr) { lua_pushnil(L); return 1; }

    const auto& players = g_netMgr->GetRemotePlayers();
    auto it = players.find(static_cast<uint8_t>(id));
    if (it == players.end() || !it->second.active) {
        lua_pushnil(L);
        return 1;
    }
    pushPlayerTable(L, it->second);
    return 1;
}

// ── network.getPlayerCount() -> integer ─────────────────────────────────────
// Returns the number of currently active remote players.
static int l_getPlayerCount(lua_State* L)
{
    if (!g_netMgr) { lua_pushinteger(L, 0); return 1; }
    int count = 0;
    for (const auto& [id, rp] : g_netMgr->GetRemotePlayers())
        if (rp.active) ++count;
    lua_pushinteger(L, static_cast<lua_Integer>(count));
    return 1;
}

// ── network.getLocalId() -> integer ─────────────────────────────────────────
// Returns our own assigned player ID. Returns 0 if not connected.
static int l_getLocalId(lua_State* L)
{
    if (!g_netMgr) { lua_pushinteger(L, 0); return 1; }
    lua_pushinteger(L, static_cast<lua_Integer>(g_netMgr->GetLocalId()));
    return 1;
}

// ── network.getMode() -> string ──────────────────────────────────────────────
// Returns "server", "client", or "none".
static int l_getMode(lua_State* L)
{
    if (!g_netMgr) { lua_pushstring(L, "none"); return 1; }
    switch (g_netMgr->GetMode()) {
        case Net::NetworkManager::Mode::Server: lua_pushstring(L, "server"); break;
        case Net::NetworkManager::Mode::Client: lua_pushstring(L, "client"); break;
        default:                                lua_pushstring(L, "none");   break;
    }
    return 1;
}

// ── network.isConnected() -> boolean ────────────────────────────────────────
// Returns true when connected as a client.
static int l_isConnected(lua_State* L)
{
    lua_pushboolean(L, (g_netMgr && g_netMgr->IsConnected()) ? 1 : 0);
    return 1;
}

// ─────────────────────────────────────────────────────────────────────────────

void setPlayersNetworkManager(Net::NetworkManager* nm)
{
    g_netMgr = nm;
}

void registerPlayers(lua_State* L, Net::NetworkManager* nm)
{
    g_netMgr = nm;

    static const luaL_Reg funcs[] = {
        {"getPlayers",      l_getPlayers},
        {"getPlayer",       l_getPlayer},
        {"getPlayerCount",  l_getPlayerCount},
        {"getLocalId",      l_getLocalId},
        {"getMode",         l_getMode},
        {"isConnected",     l_isConnected},
        {nullptr, nullptr}
    };

    luaL_newlib(L, funcs);
    lua_setglobal(L, "network");
}

} // namespace Hotones::Scripting::LuaLoader
