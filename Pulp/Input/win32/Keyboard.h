#pragma once

#ifndef _X_KEYBOARD_DEVICE_H_
#define _X_KEYBOARD_DEVICE_H_

#include "InputDeviceWin32.h"

X_NAMESPACE_DECLARE(core,
                    struct FrameInput);

X_NAMESPACE_BEGIN(input)

class XWinInput;

class XKeyboard : public XInputDeviceWin32
{
    struct AsciiVal
    {
        char lower;
        char upper;
        char caps;
        char alternate; // used for like number buttons, when caps lock is on and shit pressed EG: (caps + shift + 9) = (
    };

    typedef std::array<AsciiVal, 256> AsciiValArr;
    typedef std::array<bool, 256> BoolArr;
    typedef std::array<InputSymbol*, 256> InputSymbolPtrArr;

    X_NO_ASSIGN(XKeyboard);
    X_NO_COPY(XKeyboard);

public:
    XKeyboard(XWinInput& input, XInputCVars& vars);
    ~XKeyboard() X_FINAL;

    // IInputDevice overrides
    bool init(XBaseInput& input) X_FINAL;
    void shutDown(void) X_FINAL;
    void clearKeyState(InputEventArr& clearEvents) X_FINAL;
    // ~IInputDevice

    void processInput(const uint8_t* pData, core::FrameInput& inputFrame) X_FINAL;

private:
    void initAsciiCache(void);

    char event2Char(const InputEvent& event);
    inline bool isCHAR(const InputEvent& event);

    void processKeyboardData(const RAWKEYBOARD& rawKb, core::FrameInput& inputFrame);

private:
    AsciiValArr asciiCache_;
    BoolArr VkeyCharCache_;
    InputSymbolPtrArr pSymbol_;
};

X_NAMESPACE_END

#include "Keyboard.inl"

#endif // !_X_KEYBOARD_DEVICE_H_
