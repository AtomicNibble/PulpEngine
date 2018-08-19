#include "stdafx.h"
#include "RenderVars.h"

#include <IConsole.h>

X_NAMESPACE_BEGIN(render)

RenderVars::RenderVars() :
    pNativeRes_(nullptr),
    pRes_(nullptr)
{
    varsRegisterd_ = false;

    // defaults
    debugLayer_ = 0; // the debug override is not done here, otherwise it would not override config values.

    clearColor_ = Color(0.057f, 0.221f, 0.400f);
}

void RenderVars::registerVars(void)
{
    X_ASSERT(!varsRegisterd_, "Vars already init")(varsRegisterd_);

    ADD_CVAR_REF("r_d3d_debug_layer", debugLayer_, debugLayer_, 0, 1, core::VarFlag::RENDERER | core::VarFlag::SAVE_IF_CHANGED | core::VarFlag::RESTART_REQUIRED,
        "Enable d3d debug layer");

    ADD_CVAR_REF_COL("r_clear_color", clearColor_, clearColor_,
        core::VarFlag::RENDERER | core::VarFlag::SAVE_IF_CHANGED, "Clear color");

    pNativeRes_ = ADD_CVAR_STRING("r_native_res", "", core::VarFlag::RENDERER | core::VarFlag::READONLY,
        "The final render resolution");
    pRes_ = ADD_CVAR_STRING("r_res", "", core::VarFlag::RENDERER | core::VarFlag::READONLY,
        "The render buffer resolution, scaled to 'r_native_res'");

    varsRegisterd_ = true;
}

void RenderVars::setNativeRes(const Vec2<uint32_t>& res)
{
    core::StackString<64> buf;
    buf.appendFmt("%" PRIu32 "x%" PRIu32, res.x, res.y);

    pNativeRes_->ForceSet(buf.c_str());
}

void RenderVars::setRes(const Vec2<uint32_t>& res)
{
    core::StackString<64> buf;
    buf.appendFmt("%" PRIu32 "x%" PRIu32, res.x, res.y);

    pRes_->ForceSet(buf.c_str());
}

X_NAMESPACE_END