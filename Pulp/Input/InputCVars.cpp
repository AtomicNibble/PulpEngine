#include "StdAfx.h"
#include "InputCVars.h"

#include <ICore.h>
#include <IConsole.h>

X_NAMESPACE_BEGIN(input)


XInputCVars* g_pInputCVars = 0;

XInputCVars::XInputCVars()
{
	ADD_CVAR_REF_NO_NAME(input_debug, 0, 0, 3, 0,
		"Input debugging.");

	ADD_CVAR_REF_NO_NAME(input_mouse_pos_debug, 0, 0, 1, 0,
		"Input mouse position debugging.");

}

XInputCVars::~XInputCVars()
{
	gEnv->pConsole->UnregisterVariable("input_debug");


}


X_NAMESPACE_END