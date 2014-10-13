#pragma once

#ifndef _X_MOUSE_DEVICE_H_
#define _X_MOUSE_DEVICE_H_

#include "InputDeviceWin32.h"

X_NAMESPACE_BEGIN(input)

class XWinInput;

class XMouse : public XInputDeviceWin32
{
public:
	XMouse(XWinInput& input);
	~XMouse() X_OVERRIDE;

	// IInputDevice overrides
	virtual int GetDeviceIndex() const X_OVERRIDE{ return 0; }	//Assume only one device of this type
	virtual bool Init() X_OVERRIDE;
	virtual void ShutDown() X_OVERRIDE;
	virtual void Update(bool bFocus) X_OVERRIDE;
	virtual bool SetExclusiveMode(bool value) X_OVERRIDE;
	virtual bool IsOfDeviceType(InputDeviceType::Enum type) const X_OVERRIDE {
		return type == InputDeviceType::MOUSE; 
	}
	// ~IInputDevice

	void ProcessInput(RAWINPUT& input);

private:
	void PostEvent(InputSymbol* pSymbol);
	void PostOnlyIfChanged(InputSymbol* pSymbol, InputState::Enum newState);

	void OnButtonDown(KeyId::Enum id);
	void OnButtonUP(KeyId::Enum id);

	void ProcessMouseData(RAWMOUSE& mouse);

	InputSymbol* GetSymbol(KeyId::Enum id)
	{
		return Symbol_[id - KeyId::INPUT_MOUSE_BASE];
	}

private:
	float	mouseWheel_;

	const static int MAX_MOUSE_SYMBOLS = KeyId::MOUSE_LAST - KeyId::INPUT_MOUSE_BASE;
	static InputSymbol*	Symbol_[MAX_MOUSE_SYMBOLS];
};



X_NAMESPACE_END

#endif // !_X_MOUSE_DEVICE_H_
