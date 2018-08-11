#include "stdafx.h"
#include "GameVars.h"

#include <IConsole.h>

X_NAMESPACE_BEGIN(game)

GameVars::GameVars()
{
    userCmdDebug_ = 1;
    userCmdDrawDebug_ = 1;

    pFovVar_ = nullptr;
}

void GameVars::registerVars(void)
{
    player.registerVars();

    ADD_CVAR_REF("net_ucmd_debug", userCmdDebug_, userCmdDebug_, 0, 1, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
        "Enabled user cmd debug");

    ADD_CVAR_REF("net_ucmd_draw_debug", userCmdDrawDebug_, userCmdDrawDebug_, 0, 1, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
        "Draw user cmd debug");

    pFovVar_ = ADD_CVAR_FLOAT("cam_fov", ::toDegrees(DEFAULT_FOV), 0.01f, ::toDegrees(math<float>::PI),
        core::VarFlag::SAVE_IF_CHANGED, "camera fov");

}

X_NAMESPACE_END