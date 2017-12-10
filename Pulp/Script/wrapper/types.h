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

	struct Ref
	{
		enum Enum : int32_t
		{
			Deleted = LUA_NOREF,
			Nil = LUA_REFNIL,
		};
	};


	static_assert(Type::NIL == LUA_TNIL, "Enum mismtach");
	static_assert(Type::POINTER == LUA_TLIGHTUSERDATA, "Enum mismtach");
	static_assert(Type::BOOLEAN == LUA_TBOOLEAN, "Enum mismtach");
	static_assert(Type::NUMBER == LUA_TNUMBER, "Enum mismtach");
	static_assert(Type::STRING == LUA_TSTRING, "Enum mismtach");
	static_assert(Type::TABLE == LUA_TTABLE, "Enum mismtach");
	static_assert(Type::FUNCTION == LUA_TFUNCTION, "Enum mismtach");
	static_assert(Type::USERDATA == LUA_TUSERDATA, "Enum mismtach");
	// static_assert(Type::HANDLE == LUA_THANDLE, "Enum mismtach");



} // namespace lua

X_NAMESPACE_END