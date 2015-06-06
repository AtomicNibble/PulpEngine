#pragma once

#ifndef _X_KEYBOARD_DEVICE_H_
#define _X_KEYBOARD_DEVICE_H_

#include "InputDeviceWin32.h"

X_NAMESPACE_BEGIN(input)

class XWinInput;

class XKeyboard : public XInputDeviceWin32
{

public:
	XKeyboard(XWinInput& input);
	~XKeyboard() X_OVERRIDE;

	// IInputDevice overrides
	virtual int GetDeviceIndex() const X_OVERRIDE { return 0; }	// Assume only one keyboard
	virtual bool Init() X_OVERRIDE;
	virtual void ShutDown() X_OVERRIDE;
	virtual void Update(bool bFocus) X_OVERRIDE;
	virtual void ClearKeyState() X_OVERRIDE;
	virtual bool IsOfDeviceType(InputDeviceType::Enum type) const X_OVERRIDE{ 
		return type == InputDeviceType::KEYBOARD; 
	}
	// ~IInputDevice

	void ProcessInput(const RAWINPUTHEADER& header, const uint8_t* pData);

private:
	void initAsciiCache();

	char Event2Char(const InputEvent& event);
	inline bool IsCHAR(const InputEvent& event);

	void ProcessKeyboardData(const RAWKEYBOARD&);

	struct AsciiVal
	{
		char lower;
		char upper;
		char caps;
		char alternate; // used for like number buttons, when caps lock is on and shit pressed EG: (caps + shift + 9) = (
	};

	AsciiVal ascii_cache[256];

	static bool VkeyCharCache_[256];
	static InputSymbol*	Symbol[256];

};


X_NAMESPACE_END

#endif // !_X_KEYBOARD_DEVICE_H_
