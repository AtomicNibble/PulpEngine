#pragma once

#ifndef _X_MOUSE_DEVICE_H_
#define _X_MOUSE_DEVICE_H_

#include "InputDeviceWin32.h"

X_NAMESPACE_BEGIN(input)

class XWinInput;

class XMouse : public XInputDeviceWin32
{
	const static int MAX_MOUSE_SYMBOLS = KeyId::MOUSE_LAST - KeyId::INPUT_MOUSE_BASE;

public:
	XMouse(XWinInput& input);
	~XMouse() X_OVERRIDE;

	// IInputDevice overrides
	X_INLINE int32_t GetDeviceIndex() const X_OVERRIDE; 
	bool Init(void) X_OVERRIDE;
	void ShutDown(void) X_OVERRIDE;
	void Update(bool bFocus) X_OVERRIDE;
	bool SetExclusiveMode(bool value) X_OVERRIDE;
	X_INLINE bool IsOfDeviceType(InputDeviceType::Enum type) const X_OVERRIDE;
	// ~IInputDevice

	void ProcessInput(const uint8_t* pData);

private:
	void PostEvent(InputSymbol* pSymbol);
	void PostOnlyIfChanged(InputSymbol* pSymbol, InputState::Enum newState);

	void OnButtonDown(KeyId::Enum id);
	void OnButtonUP(KeyId::Enum id);

	void ProcessMouseData(const RAWMOUSE& mouse);

	X_INLINE InputSymbol* GetSymbol(KeyId::Enum id);

private:
	float mouseWheel_;
	static InputSymbol*	Symbol_[MAX_MOUSE_SYMBOLS];

private:
	X_NO_ASSIGN(XMouse);
	X_NO_COPY(XMouse);
};

X_NAMESPACE_END

#include "Mouse.inl"

#endif // !_X_MOUSE_DEVICE_H_
