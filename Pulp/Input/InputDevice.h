#pragma once

#ifndef _X_INPUTDEVICE_H_
#define _X_INPUTDEVICE_H_

#include <IInput.h>
#include <Containers\HashMap.h>

X_NAMESPACE_BEGIN(input)

class XInputCVars;
class XBaseInput;

class XInputDevice
{
public:
    typedef core::Array<InputEvent> InputEventArr;

public:
    XInputDevice(XBaseInput& input, XInputCVars& vars, const char* pDeviceName);
    virtual ~XInputDevice() = default;

    // IInputDevice
    X_INLINE const char* getDeviceName(void) const;
    virtual bool init(XBaseInput& input) X_ABSTRACT;
    virtual void shutDown(void) X_ABSTRACT;
    virtual void clearKeyState(InputEventArr& clearEvents) X_ABSTRACT;
    // ~IInputDevice

protected:
    XBaseInput& input_;
    XInputCVars& vars_;
    InputDeviceType::Enum deviceType_;
    bool enabled_;

private:
    core::string deviceName_; // name of the device (used for input binding)

private:
    X_NO_ASSIGN(XInputDevice);
};

X_NAMESPACE_END

#include "InputDevice.inl"

#endif // !_X_INPUTDEVICE_H_
