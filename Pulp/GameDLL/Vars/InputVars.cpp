#include "stdafx.h"
#include "InputVars.h"

#include <IConsole.h>

X_NAMESPACE_BEGIN(game)

InputVars::InputVars()
{
    toggleRun_ = 0;
    toggleCrouch_ = 0;
    toggleZoom_ = 0;
}

void InputVars::registerVars(void)
{
    ADD_CVAR_REF("input_toggle_run", toggleRun_, toggleRun_, 0, 1, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
        "Enables toggle for run");

    ADD_CVAR_REF("input_toggle_crouch", toggleCrouch_, toggleCrouch_, 0, 1, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
        "Enables toggle for crouch");

    ADD_CVAR_REF("input_toggle_zoom", toggleZoom_, toggleZoom_, 0, 1, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
        "Enables toggle for zoom");
}

X_NAMESPACE_END