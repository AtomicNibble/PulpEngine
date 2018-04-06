#pragma once

#ifndef _X_KEYBOARD_DEVICE_H_
#define _X_KEYBOARD_DEVICE_H_

#include "InputDeviceWin32.h"

X_NAMESPACE_DECLARE(core,
                    struct FrameInput;);

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
    ~XKeyboard() X_OVERRIDE;

    // IInputDevice overrides
    X_INLINE int GetDeviceIndex(void) const X_OVERRIDE;
    bool Init(void) X_OVERRIDE;
    void ShutDown(void) X_OVERRIDE;
    void Update(core::FrameData& frameData) X_OVERRIDE;
    void ClearKeyState(InputEventArr& clearEvents) X_OVERRIDE;
    X_INLINE bool IsOfDeviceType(InputDeviceType::Enum type) const X_OVERRIDE;
    // ~IInputDevice

    void ProcessInput(const uint8_t* pData, core::FrameInput& inputFrame);

private:
    void initAsciiCache(void);

    char Event2Char(const InputEvent& event);
    inline bool IsCHAR(const InputEvent& event);

    void ProcessKeyboardData(const RAWKEYBOARD& rawKb, core::FrameInput& inputFrame);

private:
    AsciiValArr asciiCache_;
    BoolArr VkeyCharCache_;
    InputSymbolPtrArr pSymbol_;
};

X_NAMESPACE_END

#include "Keyboard.inl"

#endif // !_X_KEYBOARD_DEVICE_H_
