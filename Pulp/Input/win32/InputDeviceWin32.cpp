#include "stdafx.h"
#include "InputDeviceWin32.h"

X_NAMESPACE_BEGIN(input)

XInputDeviceWin32::XInputDeviceWin32(XBaseInput& input, XInputCVars& vars, const char* pDeviceName) :
    XInputDevice(input, vars, pDeviceName)
{
}


X_NAMESPACE_END