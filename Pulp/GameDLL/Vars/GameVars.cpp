#include "stdafx.h"
#include "GameVars.h"

X_NAMESPACE_BEGIN(game)

GameVars::GameVars()
{
}

void GameVars::registerVars(void)
{
    player.registerVars();
}

X_NAMESPACE_END