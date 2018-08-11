#include "stdafx.h"
#include "InputDevice.h"

X_NAMESPACE_BEGIN(input)

XInputDevice::XInputDevice(XBaseInput& input, XInputCVars& vars, const char* pDeviceName) :
    input_(input),
    vars_(vars),
    deviceName_(pDeviceName),
    deviceType_(InputDeviceType::UNKNOWN),
    enabled_(true)
{
}


X_NAMESPACE_END