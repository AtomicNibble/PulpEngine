#pragma once



X_NAMESPACE_BEGIN(script)

namespace lua
{
	struct lua_nil_t {};
	const lua_nil_t lua_nil{};

	typedef lua_nil_t nil_t;
	const nil_t nil{};

	inline bool operator==(lua_nil_t, lua_nil_t) {
		return true;
	}
	inline bool operator!=(lua_nil_t, lua_nil_t) {
		return false;
	}

	struct ref_index 
	{
		ref_index(int idx) :
			index(idx) {
		}

		operator int() const {
			return index;
		}

		int index;
	};


	X_DECLARE_FLAGS(lib)(
		base,
		table,
		string,
		math,
		debug,
		bit32,
		package,
		os,
		io,
		// some LuaJit libs
		ffi,
		jit
	);

	typedef Flags<lib> libs;

	struct Type
	{
		enum Enum : int 
		{
			none = LUA_TNONE,
			lua_nil = LUA_TNIL,
			nil = lua_nil,
			string = LUA_TSTRING,
			number = LUA_TNUMBER,
			thread = LUA_TTHREAD,
			boolean = LUA_TBOOLEAN,
			function = LUA_TFUNCTION,
			userdata = LUA_TUSERDATA,
			lightuserdata = LUA_TLIGHTUSERDATA,
			table = LUA_TTABLE,
			poly = -0xFFFF
		};
	};


} // namespace lua

X_NAMESPACE_END