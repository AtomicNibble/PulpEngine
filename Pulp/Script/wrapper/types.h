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


	static_assert(Type::Nil == LUA_TNIL, "Enum mismtach");
	static_assert(Type::Pointer == LUA_TLIGHTUSERDATA, "Enum mismtach");
	static_assert(Type::Boolean == LUA_TBOOLEAN, "Enum mismtach");
	static_assert(Type::Number == LUA_TNUMBER, "Enum mismtach");
	static_assert(Type::String == LUA_TSTRING, "Enum mismtach");
	static_assert(Type::Table == LUA_TTABLE, "Enum mismtach");
	static_assert(Type::Function == LUA_TFUNCTION, "Enum mismtach");
	static_assert(Type::Userdata == LUA_TUSERDATA, "Enum mismtach");
	// static_assert(Type::Handle == LUA_THANDLE, "Enum mismtach");

	X_INLINE Type::Enum typeFormLua(int32_t luaType)
	{
		Type::Enum type = Type::None;

		switch (luaType)
		{
			case LUA_TNIL: type = Type::Nil; break;
			case LUA_TBOOLEAN: type = Type::Boolean; break;
			case LUA_TNUMBER: type = Type::Number; break;
			case LUA_TSTRING: type = Type::String; break;
			case LUA_TFUNCTION: type = Type::Function; break;
			case LUA_TLIGHTUSERDATA: type = Type::Pointer; break;
			case LUA_TTABLE: type = Type::Table; break;
		}

		return type;
	}

	X_INLINE bool isTypeCompatible(Type::Enum type, int32_t luaType)
	{
		switch (luaType)
		{
			case LUA_TNIL:
				if (type != Type::Nil) {
					return false;
				}
				break;
			case LUA_TBOOLEAN:
				if (type != Type::Boolean) {
					return false;
				}
				break;
			case LUA_TLIGHTUSERDATA:
				if (type != Type::Handle) {
					return false;
				}
				break;
			case LUA_TNUMBER:
				if (type != Type::Number && type != Type::Boolean) {
					return false;
				}
				break;
			case LUA_TSTRING:
				if (type != Type::String) {
					return false;
				}
				break;
			case LUA_TTABLE:
				if (type != Type::Table && type != Type::Vector) {
					return false;
				}
				break;
			case LUA_TUSERDATA:
				if (type == Type::Table) {
					return false;
				}
				break;
			case LUA_TFUNCTION:
				if (type == Type::Function) {
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