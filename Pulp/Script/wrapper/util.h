#pragma once

X_NAMESPACE_BEGIN(script)

namespace lua
{
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
		LuaStackValidator(lua_State *pL, const char *sText)
		{
			text = sText;
			L = pL;
			top = lua_gettop(L);
		}
		~LuaStackValidator()
		{
			if (top != lua_gettop(L))
			{
				X_ASSERT(false, "Lua Stack Validation Failed")();
				lua_settop(L, top);
			}
		}

		const char *text;
		lua_State *L;
		int top;
	};

	#define X_LUA_CHECK_STACK(L) LuaStackValidator __stackCheck__((L),__FUNCTION__);
#else 
	#define X_LUA_CHECK_STACK(L) X_UNUSED(L);
#endif X_ENABLE_STACK_CHECK

} // namespace lua

X_NAMESPACE_END