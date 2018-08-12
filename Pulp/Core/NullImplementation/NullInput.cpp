#include "stdafx.h"
#include "NullInput.h"

X_NAMESPACE_BEGIN(input)


void XNullInput::registerVars(void)
{
}

void XNullInput::registerCmds(void)
{
}

bool XNullInput::init(void)
{
    return true;
}

void XNullInput::update(core::FrameInput& inputFrame)
{
    X_UNUSED(inputFrame);
}

void XNullInput::shutDown(void)
{
}

bool XNullInput::job_PostInputFrame(core::V2::JobSystem& jobSys, core::FrameData& frameData)
{
    X_UNUSED(jobSys);
    X_UNUSED(frameData);
    return false;
}

void XNullInput::release(void)
{
    X_ASSERT_NOT_NULL(gEnv);
    X_ASSERT_NOT_NULL(gEnv->pArena);
    X_DELETE(this, gEnv->pArena);
}

void XNullInput::clearKeyState(void)
{
}




X_NAMESPACE_END