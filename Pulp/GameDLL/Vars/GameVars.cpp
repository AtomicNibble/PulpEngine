#include "stdafx.h"
#include "GameVars.h"

#include <IConsole.h>

X_NAMESPACE_BEGIN(game)

GameVars::GameVars()
{
    userCmdDebug_ = 1;
    userCmdDrawDebug_ = 1;
}

void GameVars::registerVars(void)
{
    player.registerVars();

    ADD_CVAR_REF("net_ucmd_debug", userCmdDebug_, userCmdDebug_, 0, 1, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
        "Enabled user cmd debug");

    ADD_CVAR_REF("net_ucmd_draw_debug", userCmdDrawDebug_, userCmdDrawDebug_, 0, 1, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
        "Draw user cmd debug");

}

X_NAMESPACE_END