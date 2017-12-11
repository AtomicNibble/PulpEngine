#include "stdafx.h"
#include "ScriptVars.h"

#include <IConsole.h>

X_NAMESPACE_BEGIN(script)


ScriptVars::ScriptVars()
{

}


void ScriptVars::registerVars(void)
{
	ADD_CVAR_REF("script_debug", debug_, 0, 0, 2, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"Enable script debug msg's 1=enabled 2=verbose");

	ADD_CVAR_REF("script_gc_step_size", gcStepSize_, 2, 0, 128, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"Set the gc step size");
}

X_NAMESPACE_END