#include "stdafx.h"
#include "util.h"

X_NAMESPACE_BEGIN(script)

namespace lua
{
	namespace
	{
		const char* g_StackLevel[] =
		{
			"",
			"  ",
			"    ",
			"      ",
			"        ",
			"          ",
			"            ",
			"              ",
		};

		const int max_stack_lvl = X_ARRAY_SIZE(g_StackLevel);


	} // namespace


	void dumpCallStack(lua_State *L)
	{
		lua_Debug ar;
		core::zero_object(ar);

		X_LOG_BULLET;
		X_ERROR("ScriptError", "CallStack");

		int level = 0;
		while (lua_getstack(L, level++, &ar))
		{
			level = core::Min(level, max_stack_lvl - 1);

			int res = lua_getinfo(L, "lnS", &ar);
			X_UNUSED(res); // TODO
			if (ar.name)
			{
				X_LOG0("ScriptError", "%s> %s, (%s: %i)",
					g_StackLevel[level], ar.name, ar.short_src, ar.currentline);
			}
			else
			{
				X_LOG0("ScriptError", "%s> (null) (%s: %i)",
					g_StackLevel[level], ar.short_src, ar.currentline);
			}
		}
	}

	int32_t myLuaPanic(lua_State *L)
	{
		X_UNUSED(L);
		X_ERROR("ScriptError", "------------------------------------------");
		X_ERROR("ScriptError", "Lua Panic");

		dumpCallStack(L);
		return 0;
	}

	int32_t myErrorHandler(lua_State *L)
	{
		const char* pErr = stack::as_string(L, 1);

		X_ERROR("ScriptError", "------------------------------------------");
		{
			X_LOG0("ScriptError", "%s", pErr);

			dumpCallStack(L);
		}

		X_ERROR("ScriptError", "------------------------------------------");
		return 0;
	}


	void recursiveTableDump(lua_State* L, int idx, int level, IRecursiveLuaDump* pSink, std::set<const void*>& tables)
	{
		X_LUA_CHECK_STACK(L);

		const char* pKey = nullptr;
		int key = 0;

		const void* pTable = stack::as_pointer(L, idx);
		if (tables.find(pTable) != tables.end()) {
			// This table was already dumped.
			return;
		}

		tables.insert(pTable);

		stack::pushnil(L);
		while (lua_next(L, idx) != 0)
		{
			// `key' is at index -2 and `value' at index -1
			if (stack::get_type(L, -2) == LUA_TSTRING) {
				pKey = stack::as_string(L, -2);
			}
			else
			{
				pKey = nullptr;
				key = stack::as_int(L, -2); // key index.
			}

			int type = stack::get_type(L);
			switch (type)
			{
				case LUA_TNIL:
					break;
				case LUA_TTABLE:
				{
					if (!(pKey != nullptr && level == 0 && strcmp(pKey, "_G") == 0))
					{
						pSink->onBeginTable(level, pKey, key);
						recursiveTableDump(L, lua_gettop(L), level + 1, pSink, tables);
						pSink->onEndTable(level);
					}
				}
				break;
				default:
				{
					ScriptValue any;
					XScriptSys::toAny(L, any, -1);
					pSink->onElement(level, pKey, key, any);
				}
				break;
			}

			stack::pop(L);
		}
	}

	void recursiveTableDump(lua_State* L, int idx, int level, IRecursiveLuaDump* pSink)
	{
		std::set<const void*> tables;

		recursiveTableDump(L, idx, level, pSink, tables);
	}


} // namespace lua

X_NAMESPACE_END