#include "stdafx.h"
#include "EffectVars.h"

#include <IConsole.h>

X_NAMESPACE_BEGIN(engine)


EffectVars::EffectVars()
{

}

void EffectVars::registerVars(void)
{
	ADD_CVAR_REF("efx_drawAxis", axisExtent_, 0, 0, 128,
		core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
		"Draw effect axis");


}



X_NAMESPACE_END