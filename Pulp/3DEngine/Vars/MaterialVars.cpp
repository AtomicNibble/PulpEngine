#include "stdafx.h"
#include "MaterialVars.h"

#include <IConsole.h>

X_NAMESPACE_BEGIN(engine)

MaterialVars::MaterialVars()
{
    // defaults
    maxActiveLoadReq_ = 32;
}

void MaterialVars::registerVars(void)
{
    ADD_CVAR_REF("mat_load_req_max", maxActiveLoadReq_, maxActiveLoadReq_, 0, 1024 * 64,
        core::VarFlag::SYSTEM | core::VarFlag::SAVE_IF_CHANGED | core::VarFlag::RESTART_REQUIRED,
        "Max active load requests for materials. 0=disabled");
}

X_NAMESPACE_END