#include "stdafx.h"
#include "state.h"

X_NAMESPACE_BEGIN(script)

namespace lua
{
	namespace
	{
		void* luaAlloc(void *ud, void *ptr, size_t osize, size_t nsize)
		{
			core::MemoryArenaBase* arena = reinterpret_cast<core::MemoryArenaBase*>(ud);

			if (nsize == 0) {
				if (ptr) {
					X_DELETE_ARRAY(static_cast<char*>(ptr), arena);
				}
				return nullptr;
			}

			void* pNew = X_NEW_ARRAY(char, nsize, arena, "LuaData");
			if (ptr) {
				memcpy(pNew, ptr, osize);
				X_DELETE_ARRAY(static_cast<char*>(ptr), arena);
			}

#if X_64
			if (reinterpret_cast<uintptr_t>(pNew) != static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pNew) & 0xFFFFFFFF)) {
				X_ASSERT(false, "Must be in first 32 bits")(pNew);
			}
#endif // X_64

			return pNew;
		}

		int lua_panic(lua_State* L)
		{
			X_LOG_BULLET;
			X_ERROR("ScriptError", "--- panic ----");

			size_t messagesize;
			const char* pMsg = lua_tolstring(L, -1, &messagesize);
			if (pMsg) {	
				X_ERROR("ScriptError", "Msg: %.*s", messagesize, pMsg);
			}

			lua_settop(L, 0);
			return 0;
		}

	} // namespace

	State::State(core::MemoryArenaBase* arena) :
#if X_64
		unique_base(luaL_newstate(), lua_close),
#else
		unique_base(lua_newstate(luaAlloc, X_ASSERT_NOT_NULL(arena)), lua_close),
#endif
		StateView(unique_base::get()),
		arena_(arena)
	{
		setPanic(lua_panic);
	}

	State& State::operator=(State&& that) {
		StateView::operator=(std::move(that));
		unique_base::operator=(std::move(that));
		return *this;
	}

	State::~State() {

	}

} // namespace lua

X_NAMESPACE_END