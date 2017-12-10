#pragma once

#include "types.h"

X_NAMESPACE_BEGIN(script)

namespace lua
{

	class StateView
	{
	public:
		StateView(lua_State* Ls);

		void openLibs(libs l);
		void setPanic(lua_CFunction panic);

		X_INLINE lua_State* luaState(void) const;
		X_INLINE operator lua_State*() const;

		template <typename Sig, typename... Args, typename Key>
		void setFunction(Key&& key, Args&&... args) {
			global_.setFunction<Sig>(std::forward<Key>(key), std::forward<Args>(args)...);
		}

		template <typename... Args, typename Key>
		void setFunction(Key&& key, Args&&... args) {
			global_.setFunction(std::forward<Key>(key), std::forward<Args>(args)...);
		}


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

} // namespace lua

X_NAMESPACE_END