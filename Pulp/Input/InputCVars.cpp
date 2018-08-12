#include "StdAfx.h"
#include "InputCVars.h"

#include <ICore.h>
#include <IConsole.h>

X_NAMESPACE_BEGIN(input)

XInputCVars::XInputCVars()
{
    inputDebug_ = 0;
    inputMousePosDebug_ = 0;
    scrollLines_ = 1;
}

XInputCVars::~XInputCVars()
{

}

void XInputCVars::registerVars(void)
{
    ADD_CVAR_REF("input_debug", inputDebug_, 0, 0, 3, core::VarFlags::SYSTEM,
        "Input debugging");

    ADD_CVAR_REF("input_mouse_pos_debug", inputMousePosDebug_, 0, 0, 1, core::VarFlags::SYSTEM,
        "Input mouse position debugging");
}

X_NAMESPACE_END