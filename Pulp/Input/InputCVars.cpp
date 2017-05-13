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
	gEnv->pConsole->UnregisterVariable("input_debug");

}


void XInputCVars::registerVars(void)
{
	ADD_CVAR_REF_NO_NAME(inputDebug_, 0, 0, 3, core::VarFlags::SYSTEM,
		"Input debugging");

	ADD_CVAR_REF_NO_NAME(inputMousePosDebug_, 0, 0, 1, core::VarFlags::SYSTEM,
		"Input mouse position debugging");

}

X_NAMESPACE_END