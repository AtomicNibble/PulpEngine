#include "stdafx.h"
#include "EffectVars.h"

#include <IConsole.h>

X_NAMESPACE_BEGIN(engine)

EffectVars::EffectVars()
{
}

void EffectVars::registerVars(void)
{
    ADD_CVAR_REF("efx_draw_debug", drawDebug_, 0, 0, 1,
        core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
        "Enable effect debug drawing");

    ADD_CVAR_REF("efx_draw_elem_rect", drawElemRect_, 0, 0, 1,
        core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
        "Draw bounds of elems");

    ADD_CVAR_REF("efx_draw_axis_scale", axisExtent_, 0, 0, 128,
        core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED,
        "Draw effect axis");
}

X_NAMESPACE_END