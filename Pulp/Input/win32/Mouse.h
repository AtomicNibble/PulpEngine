#pragma once

#ifndef _X_MOUSE_DEVICE_H_
#define _X_MOUSE_DEVICE_H_

#include "InputDeviceWin32.h"

X_NAMESPACE_DECLARE(core,
                    struct FrameInput);

X_NAMESPACE_BEGIN(input)

class XWinInput;

class XMouse : public XInputDeviceWin32
{
    const static int32_t MAX_MOUSE_SYMBOLS = KeyId::MOUSE_LAST - KeyId::INPUT_MOUSE_BASE;

    typedef std::array<InputSymbol*, MAX_MOUSE_SYMBOLS> InputSymbolPtrArr;

    X_NO_ASSIGN(XMouse);
    X_NO_COPY(XMouse);

public:
    XMouse(XWinInput& input, XInputCVars& vars);
    ~XMouse() X_FINAL;

    // XInputDevice overrides
    bool init(XBaseInput& input) X_FINAL;
    void shutDown(void) X_FINAL;
    void clearKeyState(InputEventArr& clearEvents) X_FINAL;
    // ~XInputDevice

    void processInput(const uint8_t* pData, core::FrameInput& inputFrame) X_FINAL;

private:
    void postEvent(InputSymbol* pSymbol, core::FrameInput& inputFrame);
    void postOnlyIfChanged(InputSymbol* pSymbol, InputState::Enum newState, core::FrameInput& inputFrame);

    void onButtonDown(KeyId::Enum id, core::FrameInput& inputFrame);
    void onButtonUP(KeyId::Enum id, core::FrameInput& inputFrame);

    void processMouseData(const RAWMOUSE& mouse, core::FrameInput& inputFrame);

    X_INLINE InputSymbol* getSymbol(KeyId::Enum id);

private:
    float mouseWheel_;
    InputSymbolPtrArr pSymbol_;
};

X_NAMESPACE_END

#include "Mouse.inl"

#endif // !_X_MOUSE_DEVICE_H_
