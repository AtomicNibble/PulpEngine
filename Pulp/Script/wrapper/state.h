#pragma once

#include "state_view.h"
#include <memory>

X_NAMESPACE_BEGIN(script)

namespace lua
{
	class State : private std::unique_ptr<lua_State, void(*)(lua_State*)>, public StateView
	{
		typedef std::unique_ptr<lua_State, void(*)(lua_State*)> unique_base;

	public:
		State(core::MemoryArenaBase* arena);
		State(const State&) = delete;
		State(State&&) = default;
		~State();

		State& operator=(const State&) = delete;
		State& operator=(State&& that);

	private:
		core::MemoryArenaBase* arena_;
	};



} // namespace lua

X_NAMESPACE_END