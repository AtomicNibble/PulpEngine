#pragma once

#ifndef _X_INPUTDEVICE_WIN32_H_
#define _X_INPUTDEVICE_WIN32_H_

#include <IInput.h>
#include "InputDevice.h"

// #include <string>
// #include <map>



X_NAMESPACE_BEGIN(input)

class XInputDeviceWin32 : public XInputDevice
{
public:
	XInputDeviceWin32(IInput& input, const char* deviceName);
	virtual ~XInputDeviceWin32() X_OVERRIDE;


	virtual void Update(bool bFocus)X_OVERRIDE;
	virtual void ProcessInput(RAWINPUT& input) X_ABSTRACT;
	virtual void ShutDown() X_ABSTRACT;

	// ~IInputDevice
};

X_NAMESPACE_END

#endif // !_X_INPUTDEVICE_WIN32_H_
