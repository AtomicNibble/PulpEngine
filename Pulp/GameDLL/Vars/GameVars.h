#pragma once

#include "PlayerVars.h"

X_NAMESPACE_BEGIN(game)

class GameVars
{
public:
    GameVars();
    ~GameVars() = default;

    void registerVars(void);

    PlayerVars player;
};

X_NAMESPACE_END
