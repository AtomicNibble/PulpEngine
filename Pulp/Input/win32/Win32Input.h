#pragma once

#ifndef _X_INPUT_WIN32_H_
#define _X_INPUT_WIN32_H_

#include "BaseInput.h"
#include "IFrameData.h"

#include <Util\UniquePointer.h>

struct ICore;

X_NAMESPACE_BEGIN(input)

class XInputDeviceWin32;
class XKeyboard;
class XMouse;

class XWinInput : public XBaseInput
{
    static const size_t ENTRY_HDR_SIZE = sizeof(RAWINPUTHEADER);
    static const size_t ENTRY_MOUSE_SIZE = sizeof(RAWMOUSE) + ENTRY_HDR_SIZE;
    static const size_t ENTRY_KEYBOARD_SIZE = sizeof(RAWKEYBOARD) + ENTRY_HDR_SIZE;
    static const size_t ENTRY_HID_SIZE = sizeof(RAWHID) + ENTRY_HDR_SIZE;

    static const size_t MIN_ENTRY_SIZE = core::Min(
        core::Min(ENTRY_MOUSE_SIZE, ENTRY_KEYBOARD_SIZE),
        ENTRY_HID_SIZE);
    static const size_t MAX_ENTRY_SIZE = core::Max(
        core::Max(ENTRY_MOUSE_SIZE, ENTRY_KEYBOARD_SIZE),
        ENTRY_HID_SIZE);

    static const size_t BUF_NUM = 0x20;
    static const size_t MAX_ENTRIES_IN_BUF = (sizeof(RAWINPUT) * BUF_NUM) / MIN_ENTRY_SIZE;

    typedef core::FixedArray<const RAWKEYBOARD*, MAX_ENTRIES_IN_BUF> KeyboardDataArr;
    typedef core::FixedArray<const RAWMOUSE*, MAX_ENTRIES_IN_BUF> MouseDataArr;

public:
    XWinInput(core::MemoryArenaBase* arena, HWND hWnd);
    ~XWinInput() X_FINAL;

    // IInput overrides
    void registerVars(void) X_FINAL;
    void registerCmds(void) X_FINAL;

    bool init(void) X_FINAL;
    void shutDown(void) X_FINAL;
    void release(void) X_FINAL;

    void update(core::FrameInput& inputFrame) X_FINAL;

    void clearKeyState(void) X_FINAL;
    // ~IInput

    X_INLINE HWND getHWnd(void) const;

private:
    BOOL isWow64_;
    HWND hWnd_;

    XKeyboard* pKeyBoard_;
    XMouse* pMouse_;
};

X_NAMESPACE_END

#include "Win32Input.inl"

#endif // !_X_INPUT_WIN32_H_
