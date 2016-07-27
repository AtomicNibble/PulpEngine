#include "stdafx.h"
#include "RenderVars.h"

#include <IConsole.h>

X_NAMESPACE_BEGIN(render)

RenderVars::RenderVars()
{
	// defaults
	drawAux_ = 0;
}


void RenderVars::registerVars(void)
{
	ADD_CVAR_REF("r_draw_aux", drawAux_, drawAux_, 0, 1, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"Enable drawing of aux shapes");

}


X_NAMESPACE_END