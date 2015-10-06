#include "stdafx.h"
#include "InputDeviceWin32.h"

X_NAMESPACE_BEGIN(input)



XInputDeviceWin32::XInputDeviceWin32(IInput& input, const char* deviceName) 
: XInputDevice(input, deviceName)
{

}

XInputDeviceWin32::~XInputDeviceWin32()
{

}

void XInputDeviceWin32::Update(bool focus)
{
	X_ASSERT_UNREACHABLE();
	X_UNUSED(focus);
}




X_NAMESPACE_END