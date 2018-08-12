#pragma once

#ifndef _X_NULL_INPUT_H_
#define _X_NULL_INPUT_H_

#include <IInput.h>

X_NAMESPACE_BEGIN(input)

class XNullInput : public IInput
{
public:
    virtual void registerVars(void) X_OVERRIDE;
    virtual void registerCmds(void) X_OVERRIDE;

    virtual bool init(void) X_OVERRIDE;
    virtual void shutDown(void) X_OVERRIDE;
    virtual void release(void) X_OVERRIDE;

    virtual bool job_PostInputFrame(core::V2::JobSystem& jobSys, core::FrameData& frameData) X_OVERRIDE;
    virtual void update(core::FrameInput& inputFrame) X_OVERRIDE;
    virtual void clearKeyState(void) X_OVERRIDE;
};

X_NAMESPACE_END

#endif // ! _X_NULL_INPUT_H_
