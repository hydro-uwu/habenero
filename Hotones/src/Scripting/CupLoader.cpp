// Minimal Lua runtime integration for the project.
#include <filesystem>
#include <fstream>
#include <iostream>
#include "../include/Scripting/CupLoader.hpp"
#include "../include/Scripting/LuaLoader/Rendering.hpp"

#include <lua.hpp>

using namespace std;

namespace Hotones::Scripting {

CupLoader::CupLoader()
	: L(nullptr)
{
}

CupLoader::~CupLoader()
{
	if (L) lua_close(L);
}

bool CupLoader::init()
{
	L = luaL_newstate();
	if (!L) return false;
	luaL_openlibs(L);

	// Register engine libraries
	// registerRendering is in namespace Hotones::Scripting::LuaLoader
	Hotones::Scripting::LuaLoader::registerRendering(L);

	return true;
}

bool CupLoader::loadScript(const std::string &path)
{
	if (!L) return false;
	if (!std::filesystem::exists(path)) return false;
	int status = luaL_loadfile(L, path.c_str());
	if (status != LUA_OK) {
		const char* msg = lua_tostring(L, -1);
		std::cerr << "Lua load error: " << (msg ? msg : "<unknown>") << std::endl;
		lua_pop(L,1);
		return false;
	}
	// leave chunk on stack for `run` or call immediately
	return true;
}

bool CupLoader::run()
{
	if (!L) return false;
	int status = lua_pcall(L, 0, LUA_MULTRET, 0);
	if (status != LUA_OK) {
		const char* msg = lua_tostring(L, -1);
		std::cerr << "Lua runtime error: " << (msg ? msg : "<unknown>") << std::endl;
		lua_pop(L,1);
		return false;
	}
	return true;
}

lua_State* CupLoader::state()
{
	return L;
}

} // namespace Hotones::Scripting
