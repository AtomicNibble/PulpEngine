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
	virtual bool Init(void) X_OVERRIDE;
	virtual void ShutDown(void) X_OVERRIDE;
	virtual void Update(bool bFocus) X_OVERRIDE;
	virtual bool SetExclusiveMode(bool value) X_OVERRIDE;
	virtual bool IsOfDeviceType(InputDeviceType::Enum type) const X_OVERRIDE {
		return type == InputDeviceType::MOUSE; 
	}
	// ~IInputDevice

	void ProcessInput(const RAWINPUTHEADER& header, const uint8_t* pData);

private:
	void PostEvent(InputSymbol* pSymbol);
	void PostOnlyIfChanged(InputSymbol* pSymbol, InputState::Enum newState);

	void OnButtonDown(KeyId::Enum id);
	void OnButtonUP(KeyId::Enum id);

	void ProcessMouseData(const RAWMOUSE& mouse);

	InputSymbol* GetSymbol(KeyId::Enum id)
	{
		return Symbol_[id - KeyId::INPUT_MOUSE_BASE];
	}

private:
	float mouseWheel_;

	const static int MAX_MOUSE_SYMBOLS = KeyId::MOUSE_LAST - KeyId::INPUT_MOUSE_BASE;
	static InputSymbol*	Symbol_[MAX_MOUSE_SYMBOLS];

private:
	X_NO_ASSIGN(XMouse);
	X_NO_COPY(XMouse);
};



X_NAMESPACE_END

#endif // !_X_MOUSE_DEVICE_H_
