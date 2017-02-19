#include "StdAfx.h"
#include "InputCVars.h"

#include <ICore.h>
#include <IConsole.h>

X_NAMESPACE_BEGIN(input)


XInputCVars* g_pInputCVars = 0;

XInputCVars::XInputCVars()
{
	input_debug = 0;
	input_mouse_pos_debug = 0;
	scrollLines = 1;
}

XInputCVars::~XInputCVars()
{
	gEnv->pConsole->UnregisterVariable("input_debug");

}


void XInputCVars::registerVars(void)
{
	ADD_CVAR_REF_NO_NAME(input_debug, 0, 0, 3, core::VarFlags::SYSTEM,
		"Input debugging");

	ADD_CVAR_REF_NO_NAME(input_mouse_pos_debug, 0, 0, 1, core::VarFlags::SYSTEM,
		"Input mouse position debugging");

}

X_NAMESPACE_END