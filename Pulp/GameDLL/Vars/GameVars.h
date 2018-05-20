#pragma once

#include "PlayerVars.h"

X_NAMESPACE_BEGIN(game)

class GameVars
{
public:
    GameVars();
    ~GameVars() = default;

    void registerVars(void);

    X_INLINE bool userCmdDrawDebug(void) const;

    PlayerVars player;

private:
    int32_t userCmdDrawDebug_;
};

X_NAMESPACE_END

#include "GameVars.inl"