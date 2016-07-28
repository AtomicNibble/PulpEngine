#include "stdafx.h"
#include "RenderVars.h"

#include <IConsole.h>

X_NAMESPACE_BEGIN(render)

RenderVars::RenderVars()
{
	// defaults
	drawAux_ = 0;

	r_clear_color = Color(0.057f, 0.221f, 0.400f);
}


void RenderVars::registerVars(void)
{
	ADD_CVAR_REF("r_draw_aux", drawAux_, drawAux_, 0, 1, core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"Enable drawing of aux shapes");

	ADD_CVAR_REF_COL_NO_NAME(r_clear_color, r_clear_color,
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED, "Clear color");
}


X_NAMESPACE_END