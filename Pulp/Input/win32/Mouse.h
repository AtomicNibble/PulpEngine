#pragma once

#ifndef _X_MOUSE_DEVICE_H_
#define _X_MOUSE_DEVICE_H_

#include "InputDeviceWin32.h"

X_NAMESPACE_DECLARE(core,
	struct FrameInput;
);

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
	void Update(core::FrameData& frameData) X_OVERRIDE;
	X_INLINE bool IsOfDeviceType(InputDeviceType::Enum type) const X_OVERRIDE;
	// ~IInputDevice

	void ProcessInput(const uint8_t* pData, core::FrameInput& inputFrame);

private:
	void PostEvent(InputSymbol* pSymbol, core::FrameInput& inputFrame);
	void PostOnlyIfChanged(InputSymbol* pSymbol, InputState::Enum newState, core::FrameInput& inputFrame);

	void OnButtonDown(KeyId::Enum id, core::FrameInput& inputFrame);
	void OnButtonUP(KeyId::Enum id, core::FrameInput& inputFrame);

	void ProcessMouseData(const RAWMOUSE& mouse, core::FrameInput& inputFrame);

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
