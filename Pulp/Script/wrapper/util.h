#pragma once

X_NAMESPACE_BEGIN(script)

namespace lua
{
	void dumpCallStack(lua_State *L);
	int32_t myLuaPanic(lua_State *L);
	int32_t myErrorHandler(lua_State *L);


	struct IRecursiveLuaDump
	{
		virtual ~IRecursiveLuaDump() {}
		virtual void onElement(int level, const char* pKey, int key, ScriptValue& value) X_ABSTRACT;
		virtual void onBeginTable(int level, const char* pKey, int key) X_ABSTRACT;
		virtual void onEndTable(int level) X_ABSTRACT;
	};

	void recursiveTableDump(lua_State* L, int idx, int level, IRecursiveLuaDump* pSink);


	struct LuaStackGuard
	{
		LuaStackGuard(lua_State *p)
		{
			pLS_ = p;
			top_ = lua_gettop(pLS_);
		}
		~LuaStackGuard()
		{
			lua_settop(pLS_, top_);
		}
	private:
		int top_;
		lua_State* pLS_;
	};
	
#if X_ENABLE_STACK_CHECK

	struct LuaStackValidator
	{
		LuaStackValidator(lua_State* pL, const char* sText)
		{
			pText_ = sText;
			L_ = pL;
			top_ = lua_gettop(L_);
		}
		~LuaStackValidator()
		{
			if (top_ != lua_gettop(L_))
			{
				X_ASSERT(false, "Lua Stack Validation Failed")(pText_);
				lua_settop(L_, top_);
			}
		}

		const char* pText_;
		lua_State *L_;
		int top_;
	};

	#define X_LUA_CHECK_STACK(L) LuaStackValidator __stackCheck__((L),__FUNCTION__);
#else 
	#define X_LUA_CHECK_STACK(L) X_UNUSED(L);
#endif X_ENABLE_STACK_CHECK

} // namespace lua

X_NAMESPACE_END