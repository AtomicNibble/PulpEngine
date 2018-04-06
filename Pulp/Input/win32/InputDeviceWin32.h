#pragma once

#ifndef _X_INPUTDEVICE_WIN32_H_
#define _X_INPUTDEVICE_WIN32_H_

#include <IInput.h>
#include "InputDevice.h"

X_NAMESPACE_DECLARE(core,
                    struct FrameInput;);

X_NAMESPACE_BEGIN(input)

class XInputDeviceWin32 : public XInputDevice
{
public:
    XInputDeviceWin32(IInput& input, XInputCVars& vars, const char* pDeviceName);
    virtual ~XInputDeviceWin32() X_OVERRIDE;

    virtual void Update(core::FrameData& frameData) X_OVERRIDE;
    virtual void ProcessInput(const uint8_t* pData, core::FrameInput& inputFrame) X_OVERRIDE;
    virtual void ShutDown(void) X_OVERRIDE;

    // ~IInputDevice

private:
    X_NO_ASSIGN(XInputDeviceWin32);
    X_NO_COPY(XInputDeviceWin32);
};

X_NAMESPACE_END

#endif // !_X_INPUTDEVICE_WIN32_H_
