#include <iostream>
#include <lua.hpp>
#include "../../include/Scripting/LuaLoader/Rendering.hpp"
#include "../../include/GFX/Renderer.hpp"

using namespace std;

namespace Hotones::Scripting::LuaLoader {

static int l_drawText(lua_State* L)
{
    const char* text = luaL_checkstring(L, 1);
    int x = (int)luaL_checkinteger(L, 2);
    int y = (int)luaL_checkinteger(L, 3);
    int size = (int)luaL_optinteger(L, 4, 20);
    int r = (int)luaL_optinteger(L, 5, 255);
    int g = (int)luaL_optinteger(L, 6, 255);
    int b = (int)luaL_optinteger(L, 7, 255);
    int a = (int)luaL_optinteger(L, 8, 255);
    Hotones::GFX::Renderer::DrawText(text, x, y, size, r, g, b, a);
    return 0;
}

static int l_clearScreen(lua_State* L)
{
    int r = (int)luaL_optinteger(L, 1, 0);
    int g = (int)luaL_optinteger(L, 2, 0);
    int b = (int)luaL_optinteger(L, 3, 0);
    int a = (int)luaL_optinteger(L, 4, 255);
    Hotones::GFX::Renderer::ClearScreen(r, g, b, a);
    return 0;
}

static int l_drawRect(lua_State* L)
{
    int x = (int)luaL_checkinteger(L, 1);
    int y = (int)luaL_checkinteger(L, 2);
    int w = (int)luaL_checkinteger(L, 3);
    int h = (int)luaL_checkinteger(L, 4);
    int r = (int)luaL_optinteger(L, 5, 255);
    int g = (int)luaL_optinteger(L, 6, 255);
    int b = (int)luaL_optinteger(L, 7, 255);
    int a = (int)luaL_optinteger(L, 8, 255);
    Hotones::GFX::Renderer::DrawRect(x, y, w, h, r, g, b, a);
    return 0;
}

void registerRendering(lua_State* L)
{
    static const luaL_Reg funcs[] = {
        {"drawText", l_drawText},
        {"clearScreen", l_clearScreen},
        {"drawRect", l_drawRect},
        {NULL, NULL}
    };

    luaL_newlib(L, funcs);
    lua_setglobal(L, "render");
}

} // namespace Hotones::Scripting::LuaLoader
