#pragma once

X_NAMESPACE_BEGIN(script)


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

#if X_DEBUG

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
#else //_DEBUG
#define X_LUA_CHECK_STACK(L) X_UNUSED(L);
#endif //_DEBUG


X_NAMESPACE_END