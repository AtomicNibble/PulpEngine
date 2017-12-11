#pragma once



X_NAMESPACE_BEGIN(script)

namespace lua
{
	#ifndef LUA_OK
	#define LUA_OK 0
	#endif

	typedef int32_t RefId;

	X_DECLARE_FLAGS(lib)(
		Base,
		Table,
		String,
		Math,
		Debug,
		Bit32,
		Package,
		Os,
		Io,
		// some LuaJit libs
		Ffi,
		Jit
	);

	typedef Flags<lib> libs;

	X_DECLARE_ENUM(CallResult)(
		Ok,
		Yield,
		RunTime,
		Syntax,
		Memory,
		Handler,
		File
	);

	static_assert(CallResult::Ok == LUA_OK, "Enum mismtach");
	static_assert(CallResult::Yield == LUA_YIELD, "Enum mismtach");
	static_assert(CallResult::RunTime == LUA_ERRRUN, "Enum mismtach");
	static_assert(CallResult::Syntax == LUA_ERRSYNTAX, "Enum mismtach");
	static_assert(CallResult::Memory == LUA_ERRMEM, "Enum mismtach");
	static_assert(CallResult::Handler == LUA_ERRERR, "Enum mismtach");
	static_assert(CallResult::File == LUA_ERRFILE, "Enum mismtach");


	X_DECLARE_ENUM(LoadResult)(
		Ok,
		_blank1,
		_blank2,
		Syntax,
		Memory,
		_blank3,
		File
	);

	static_assert(LoadResult::Ok == LUA_OK, "Enum mismtach");
	static_assert(LoadResult::Syntax == LUA_ERRSYNTAX, "Enum mismtach");
	static_assert(LoadResult::Memory == LUA_ERRMEM, "Enum mismtach");
	static_assert(LoadResult::File == LUA_ERRFILE, "Enum mismtach");

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

	X_INLINE Type::Enum typeFormLua(int32_t luaType)
	{
		Type::Enum type = Type::NONE;

		switch (luaType)
		{
			case LUA_TNIL: type = Type::NIL; break;
			case LUA_TBOOLEAN: type = Type::BOOLEAN; break;
			case LUA_TNUMBER: type = Type::NUMBER; break;
			case LUA_TSTRING: type = Type::STRING; break;
			case LUA_TFUNCTION: type = Type::FUNCTION; break;
			case LUA_TLIGHTUSERDATA: type = Type::POINTER; break;
			case LUA_TTABLE: type = Type::TABLE; break;
		}

		return type;
	}

	X_INLINE bool isTypeCompatible(Type::Enum type, int32_t luaType)
	{
		switch (luaType)
		{
			case LUA_TNIL:
				if (type != Type::NIL) {
					return false;
				}
				break;
			case LUA_TBOOLEAN:
				if (type != Type::BOOLEAN) {
					return false;
				}
				break;
			case LUA_TLIGHTUSERDATA:
				if (type != Type::HANDLE) {
					return false;
				}
				break;
			case LUA_TNUMBER:
				if (type != Type::NUMBER && type != Type::BOOLEAN) {
					return false;
				}
				break;
			case LUA_TSTRING:
				if (type != Type::STRING) {
					return false;
				}
				break;
			case LUA_TTABLE:
				if (type != Type::TABLE && type != Type::VECTOR) {
					return false;
				}
				break;
			case LUA_TUSERDATA:
				if (type == Type::TABLE) {
					return false;
				}
				break;
			case LUA_TFUNCTION:
				if (type == Type::FUNCTION) {
					return false;
				}
				break;
			case LUA_TTHREAD:
				break;
		}

		return true;
	}

} // namespace lua

X_NAMESPACE_END