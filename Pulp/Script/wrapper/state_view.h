#pragma once

#include "types.h"

X_NAMESPACE_BEGIN(script)

namespace lua
{
	X_INLINE int32_t functionHandleToRef(ScriptFunctionHandle ref)
	{
		return safe_static_cast<int32_t>(reinterpret_cast<intptr_t>(ref));
	}

	X_INLINE ScriptFunctionHandle refToScriptHandle(int32_t ref)
	{
		return reinterpret_cast<ScriptFunctionHandle>(static_cast<intptr_t>(ref));
	}


	namespace stack
	{
		// misc
		X_INLINE void checkstack(lua_State* L, int32_t requiredSlots)
		{
			luaL_checkstack(L, requiredSlots, "Not enougth stack slots");
		}

		X_INLINE bool is_empty(lua_State* L)
		{
			return lua_gettop(L) == 0;
		}

		X_INLINE int32_t num(lua_State* L)
		{
			return lua_gettop(L);
		}

		X_INLINE int32_t top(lua_State* L)
		{
			return lua_gettop(L);
		}

		X_INLINE void settop(lua_State* L, int32_t idx)
		{
			lua_settop(L, idx);
		}

		X_INLINE void move_top_to(lua_State* L, int32_t idx)
		{
			lua_insert(L, idx);
		}

		X_INLINE void remove(lua_State* L, int32_t idx)
		{
			lua_remove(L, idx);
		}

		X_INLINE void push_copy(lua_State* L, int32_t idx)
		{
			lua_pushvalue(L, idx);
		}

		X_INLINE size_t rawlen(lua_State* L, int32_t idx)
		{
			return lua_objlen(L, idx);
		}

		X_INLINE size_t rawlen(lua_State* L)
		{
			return lua_objlen(L, -1);
		}

		// types
		X_INLINE int32_t get_type(lua_State* L)
		{
			return lua_type(L, -1);
		}

		X_INLINE int32_t get_type(lua_State* L, int32_t index)
		{
			return lua_type(L, index);
		}

		// type helpers.
		X_INLINE bool isnil(lua_State* L)
		{
			return lua_isnil(L, -1);
		}

		X_INLINE bool isboolean(lua_State* L)
		{
			return lua_isboolean(L, -1);
		}

		X_INLINE bool isfunction(lua_State* L)
		{
			return lua_isfunction(L, -1);
		}

		X_INLINE bool istable(lua_State* L)
		{
			return lua_istable(L, -1);
		}

		X_INLINE bool isnumber(lua_State* L)
		{
			return lua_isnumber(L, -1) != 0;
		}

		X_INLINE bool islightuserdata(lua_State* L)
		{
			return lua_islightuserdata(L, -1);
		}


		// pop
		X_INLINE void pop(lua_State* L)
		{
			lua_pop(L, 1);
		}

		X_INLINE void pop(lua_State* L, int32_t num)
		{
			lua_pop(L, num);
		}

		X_INLINE void pop_to_global(lua_State* L, const char* pKey)
		{
			lua_setfield(L, LUA_GLOBALSINDEX, pKey);
		}

		// push
		X_INLINE void push(lua_State* L, const char* pStr, size_t len)
		{
			lua_pushlstring(L, pStr, safe_static_cast<int32_t>(len));
		}

		X_INLINE void push(lua_State* L, const char* pStr)
		{
			push(L, pStr, core::strUtil::strlen(pStr));
		}

		template<size_t N>
		X_INLINE void pushliteral(lua_State* L, const char(&buf)[N])
		{
			push(L, buf, sizeof(buf) - 1);
		}

		X_INLINE void push(lua_State* L, bool b)
		{
			lua_pushboolean(L, b);
		}

		X_INLINE void push(lua_State* L, double n)
		{
			lua_pushnumber(L, n);
		}

		X_INLINE void push(lua_State* L, int32_t n)
		{
			lua_pushnumber(L, n);
		}


		X_INLINE void push(lua_State* L, lua_CFunction fn, int32_t numAssociatedValues)
		{
			lua_pushcclosure(L, fn, numAssociatedValues);
		}

		X_INLINE void push(lua_State* L, lua_CFunction fn)
		{
			lua_pushcclosure(L, fn, 0);
		}

		X_INLINE void pushnil(lua_State* L)
		{
			lua_pushnil(L);
		}


		X_INLINE void push_global(lua_State* L, const char* pKey)
		{
			lua_getfield(L, LUA_GLOBALSINDEX, pKey);
		}

		X_INLINE bool push_global_table_value(lua_State* L, const char* pTable, const char* pKey)
		{
			push_global(L, pTable);
			if (!stack::istable(L)) {
				stack::pop(L);
				return false;
			}

			push(L, pKey);

			// Pushes global[pTable][pKey] onto the stack
			lua_gettable(L, -2);
			remove(L, -2); // remove the table, so just the value on stack.
			return true;
		}

		X_INLINE void push_table_value(lua_State* L, int32_t tableIdx, const char* pKey)
		{
			push(L, pKey);
			lua_gettable(L, tableIdx);
		}

		X_INLINE void push_table_value(lua_State* L, int32_t tableIdx, int32_t idx)
		{
			lua_rawgeti(L, tableIdx, idx);
		}


		X_INLINE void pop_value_to_table(lua_State* L, int32_t tableIdx, int32_t idx)
		{
			lua_rawseti(L, tableIdx, idx);
		}

		X_INLINE void pop_value_to_table(lua_State* L, int32_t tableIdx)
		{
			// t[k] = v
			// t = tableIdx
			// v = -1
			// k = -2
			lua_rawset(L, tableIdx);
		}


		// refrences 
		X_INLINE void push_ref(lua_State* L, int32_t ref)
		{
			lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
		}

		X_INLINE void push_ref(lua_State* L, ScriptFunctionHandle ref)
		{
			lua_rawgeti(L, LUA_REGISTRYINDEX, functionHandleToRef(ref));
		}

		X_INLINE int32_t pop_to_ref(lua_State* L)
		{
			return luaL_ref(L, LUA_REGISTRYINDEX);
		}



		// as values.
		X_INLINE const void* as_pointer(lua_State* L, int32_t idx)
		{
			return lua_topointer(L, idx);
		}

		X_INLINE const void* as_pointer(lua_State* L)
		{
			return lua_topointer(L, -1);
		}

		X_INLINE bool as_bool(lua_State* L, int32_t idx)
		{
			return lua_toboolean(L, idx) != 0;
		}

		X_INLINE bool as_bool(lua_State* L)
		{
			return lua_toboolean(L, -1) != 0;
		}

		X_INLINE double as_number(lua_State* L, int32_t idx)
		{
			return lua_tonumber(L, idx);
		}

		X_INLINE double as_number(lua_State* L)
		{
			return lua_tonumber(L, -1);
		}

		X_INLINE int32_t as_int(lua_State* L, int32_t idx)
		{
			return static_cast<int32_t>(lua_tonumber(L, idx));
		}

		X_INLINE int32_t as_int(lua_State* L)
		{
			return static_cast<int32_t>(lua_tonumber(L, -1));
		}

		X_INLINE void* as_userdata(lua_State* L, int32_t idx)
		{
			return lua_touserdata(L, idx);
		}

		X_INLINE void* as_userdata(lua_State* L)
		{
			return lua_touserdata(L, -1);
		}

		X_INLINE const char* as_string(lua_State* L, int32_t idx, size_t* size = nullptr)
		{
			return lua_tolstring(L, idx, size);
		}

		X_INLINE const char* as_string(lua_State* L, size_t* size = nullptr)
		{
			return lua_tolstring(L, -1, size);
		}

		// run

		X_INLINE void call(lua_State* L, int32_t numArgs, int32_t numResults)
		{
			lua_call(L, numArgs, numResults);
		}

		X_INLINE CallResult::Enum pcall(lua_State* L, int32_t numArgs, int32_t numResults, int32_t messageHandlerStackIndex)
		{
			return static_cast<CallResult::Enum>(lua_pcall(L, numArgs, numResults, messageHandlerStackIndex));
		}


	}

	namespace state
	{

		X_INLINE void remove_ref(lua_State* L, int32_t ref)
		{
			luaL_unref(L, LUA_REGISTRYINDEX, ref);
		}

		X_INLINE void remove_ref(lua_State* L, ScriptFunctionHandle ref)
		{
			luaL_unref(L, LUA_REGISTRYINDEX, functionHandleToRef(ref));
		}

		X_INLINE void new_table(lua_State* L)
		{
			lua_createtable(L, 0, 0);
		}

		X_INLINE void set_table_value(lua_State* L)
		{
			lua_settable(L, -3);
		}

		X_INLINE LoadResult::Enum load(lua_State* L, const char* pBegin, const char* pEnd, const char* pChunkName)
		{
			int status = luaL_loadbuffer(L, pBegin, pEnd - pBegin, pChunkName);

			return static_cast<LoadResult::Enum>(status);
		}

		X_INLINE void* newuserdata(lua_State *L, size_t size)
		{
			return lua_newuserdata(L, size);
		}

		X_INLINE void* newuserdata(lua_State *L, void* pData, size_t size)
		{
			void* pUserPtr = lua_newuserdata(L, size);
			std::memcpy(pUserPtr, pData, size);
			return pUserPtr;
		}

		X_INLINE void gc_full(lua_State* L)
		{
			lua_gc(L, LUA_GCCOLLECT, 0);
		}

		X_INLINE void gc_step(lua_State* L, int32_t size)
		{
			lua_gc(L, LUA_GCSTEP, size);
		}

		X_INLINE void gc_stop(lua_State* L)
		{
			lua_gc(L, LUA_GCSTOP, 0);
		}

		X_INLINE void gc_start(lua_State* L)
		{
			lua_gc(L, LUA_GCRESTART, 0);
		}

		

	} // namespace state



	class StateView
	{
	public:
		StateView(lua_State* Ls);

		void openLibs(libs l);
		void setPanic(lua_CFunction panic);

		X_INLINE lua_State* luaState(void) const;
		X_INLINE operator lua_State*() const;

		X_INLINE bool loadScript(const char* pBegin, const char* pEnd, const char* pChunkName);


	private:
		lua_State* L_;
	};


	X_INLINE lua_State* StateView::luaState(void) const
	{
		return L_;
	}

	X_INLINE StateView::operator lua_State*() const
	{
		return L_;
	}

	X_INLINE bool StateView::loadScript(const char* pBegin, const char* pEnd, const char* pChunkName)
	{
		auto status = state::load(L_, pBegin, pEnd, pChunkName);

		return status == LoadResult::Ok;
	}

} // namespace lua

X_NAMESPACE_END