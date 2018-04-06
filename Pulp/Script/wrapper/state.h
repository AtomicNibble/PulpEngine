#pragma once

#include "state_view.h"
#include <memory>

X_NAMESPACE_BEGIN(script)

namespace lua
{
    class State : private std::unique_ptr<lua_State, void (*)(lua_State*)>
        , public StateView
    {
        typedef std::unique_ptr<lua_State, void (*)(lua_State*)> unique_base;

        X_NO_COPY(State);
        X_NO_ASSIGN(State);

    public:
        State(core::MemoryArenaBase* arena);
        State(State&&) = default;
        ~State();

        State& operator=(State&& that);

    private:
        core::MemoryArenaBase* arena_;
    };

} // namespace lua

X_NAMESPACE_END