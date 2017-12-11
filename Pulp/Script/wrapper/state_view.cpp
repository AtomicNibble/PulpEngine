#include "stdafx.h"
#include "state_view.h"


X_NAMESPACE_BEGIN(script)

namespace lua
{
	namespace
	{
		#define lua_getfield(L, i, k) \
			(lua_getfield((L), (i), (k)), lua_type((L), -1))

		int lua_absindex(lua_State *L, int i) 
		{
			if (i < 0 && i > LUA_REGISTRYINDEX) {
				i += lua_gettop(L) + 1;
			}
			return i;
		}

		int luaL_getsubtable(lua_State *L, int i, const char *name) 
		{
			int abs_i = lua_absindex(L, i);
			luaL_checkstack(L, 3, "not enough stack slots");
			lua_pushstring(L, name);
			lua_gettable(L, abs_i);
			if (lua_istable(L, -1)) {
				return 1;
			}
			lua_pop(L, 1);
			lua_newtable(L);
			lua_pushstring(L, name);
			lua_pushvalue(L, -2);
			lua_settable(L, abs_i);
			return 0;
		}


		void luaL_requiref(lua_State *L, const char *modname, lua_CFunction openf, int glb) 
		{
			luaL_checkstack(L, 3, "not enough stack slots available");
			luaL_getsubtable(L, LUA_REGISTRYINDEX, "_LOADED");
			if (lua_getfield(L, -1, modname) == LUA_TNIL) {
				lua_pop(L, 1);
				lua_pushcfunction(L, openf);
				lua_pushstring(L, modname);
				lua_call(L, 1, 1);
				lua_pushvalue(L, -1);
				lua_setfield(L, -3, modname);
			}
			if (glb) {
				lua_pushvalue(L, -1);
				lua_setglobal(L, modname);
			}
			lua_replace(L, -2);
		}

	} // namespace


	StateView::StateView(lua_State* Ls) :
		L_(Ls)
	{

	}

	void StateView::openLibs(libs l)
	{
		if (l.IsSet(lib::Base))
		{
			luaL_requiref(L_, "base", luaopen_base, 1);
			lua_pop(L_, 1);
		}

		if (l.IsSet(lib::Table))
		{
			luaL_requiref(L_, "table", luaopen_table, 1);
			lua_pop(L_, 1);
		}

		if (l.IsSet(lib::String))
		{
			luaL_requiref(L_, "string", luaopen_string, 1);
			lua_pop(L_, 1);
		}

		if (l.IsSet(lib::Math))
		{
			luaL_requiref(L_, "math", luaopen_math, 1);
			lua_pop(L_, 1);
		}

		if (l.IsSet(lib::Debug))
		{
			luaL_requiref(L_, "debug", luaopen_debug, 1);
			lua_pop(L_, 1);
		}

		if (l.IsSet(lib::Bit32))
		{
			luaL_requiref(L_, "bit", luaopen_bit, 1);
			lua_pop(L_, 1);
		}

		if (l.IsSet(lib::Package))
		{
			luaL_requiref(L_, "package", luaopen_package, 1);
			lua_pop(L_, 1);
		}

		if (l.IsSet(lib::Os))
		{
			luaL_requiref(L_, "os", luaopen_os, 1);
			lua_pop(L_, 1);
		}

		if (l.IsSet(lib::Io))
		{
			luaL_requiref(L_, "io", luaopen_io, 1);
			lua_pop(L_, 1);
		}

		if (l.IsSet(lib::Ffi))
		{
			luaL_requiref(L_, "ffi", luaopen_ffi, 1);
			lua_pop(L_, 1);
		}

		if (l.IsSet(lib::Jit))
		{
			luaL_requiref(L_, "jit", luaopen_jit, 1);
			lua_pop(L_, 1);
		}
	}

	void StateView::setPanic(lua_CFunction panic)
	{
		lua_atpanic(L_, panic);
	}


} // namespace lua

X_NAMESPACE_END