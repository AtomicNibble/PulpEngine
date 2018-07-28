#pragma once

#ifndef _X_INPUT_WIN32_H_
#define _X_INPUT_WIN32_H_

#include "BaseInput.h"
#include "IFrameData.h"

struct ICore;

X_NAMESPACE_BEGIN(input)

class XInputDeviceWin32;

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

    bool Init(void) X_FINAL;
    void PostInit(void) X_FINAL;
    void ShutDown(void) X_FINAL;
    void release(void) X_FINAL;

    void Update(core::FrameData& frameData) X_FINAL;

    void ClearKeyState(void) X_FINAL;
    // ~IInput

    bool AddInputDevice(IInputDevice* pDevice) X_FINAL;
    bool AddInputDevice(XInputDeviceWin32* pDevice);

    X_INLINE HWND GetHWnd(void) const;

private:
    BOOL isWow64_;
    HWND hWnd_;

    XInputDeviceWin32* pKeyBoard_;
    XInputDeviceWin32* pMouse_;
};

X_NAMESPACE_END

#include "Win32Input.inl"

#endif // !_X_INPUT_WIN32_H_
