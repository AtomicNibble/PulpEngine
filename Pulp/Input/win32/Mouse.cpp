#include "stdafx.h"
#include "Mouse.h"
#include "Win32Input.h"

#include "InputCVars.h"
#include "IConsole.h"

X_NAMESPACE_BEGIN(input)

XMouse::XMouse(XWinInput& input, XInputCVars& vars) :
    XInputDeviceWin32(input, vars, "mouse")
{
    deviceType_ = InputDeviceType::MOUSE;
    mouseWheel_ = 0.f;

    pSymbol_.fill(nullptr);
}

XMouse::~XMouse()
{
}

///////////////////////////////////////////
bool XMouse::Init(void)
{
    RAWINPUTDEVICE Mouse;
    Mouse.hwndTarget = 0;
    Mouse.usUsagePage = 0x1;
    Mouse.usUsage = 0x2;
    Mouse.dwFlags = 0;

    bool res = true;

    if (!RegisterRawInputDevices(&Mouse, 1, sizeof(RAWINPUTDEVICE))) {
        core::lastError::Description Dsc;
        X_ERROR("Mouse", "Failed to register mouse: %s", core::lastError::ToString(Dsc));
        res = false;
    }

    // Load the user setting.
    UINT LinesToScroll;
    if (SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &LinesToScroll, 0)) {
        ADD_CVAR_REF("mouse_scroll_linenum", vars_.scrollLines_, LinesToScroll, 0, 1920,
            core::VarFlag::SYSTEM, "The number of lines to scroll at a time");
    }
    else {
        X_WARNING("Mouse", "Failed to retrieve sys scroll settings, defaulting to: %i", vars_.scrollLines_);
    }

    IInput& input = GetIInput();

    // init the symbols
#define DEFINE_SYM(id, name) \
    pSymbol_[id - KeyId::INPUT_MOUSE_BASE] = input.DefineSymbol(InputDeviceType::MOUSE, id, name);

    DEFINE_SYM(KeyId::MOUSE_LEFT, "MOUSE LEFT");
    DEFINE_SYM(KeyId::MOUSE_MIDDLE, "MOUSE_MIDDLE");
    DEFINE_SYM(KeyId::MOUSE_RIGHT, "MOUSE_RIGHT");
    DEFINE_SYM(KeyId::MOUSE_AUX_1, "MOUSE_AUX_1");
    DEFINE_SYM(KeyId::MOUSE_AUX_2, "MOUSE_AUX_2");
    DEFINE_SYM(KeyId::MOUSE_AUX_3, "MOUSE_AUX_3");
    DEFINE_SYM(KeyId::MOUSE_AUX_4, "MOUSE_AUX_4");
    DEFINE_SYM(KeyId::MOUSE_AUX_5, "MOUSE_AUX_5");

    DEFINE_SYM(KeyId::MOUSE_WHEELUP, "MOUSE_AUX_5");
    DEFINE_SYM(KeyId::MOUSE_WHEELDOWN, "MOUSE_AUX_5");

    DEFINE_SYM(KeyId::MOUSE_X, "MOUSE_X");
    DEFINE_SYM(KeyId::MOUSE_Y, "MOUSE_Y");
    DEFINE_SYM(KeyId::MOUSE_Z, "MOUSE_Z");

    DEFINE_SYM(KeyId::MOUSE_WHEELDOWN, "MOUSE_WHEELDOWN");
    DEFINE_SYM(KeyId::MOUSE_WHEELUP, "MOUSE_WHEELUP");

    return res;
}

void XMouse::ShutDown(void)
{
    RAWINPUTDEVICE Mouse;
    Mouse.hwndTarget = 0;
    Mouse.usUsagePage = 0x1;
    Mouse.usUsage = 0x2;
    Mouse.dwFlags = RIDEV_REMOVE;

    if (!RegisterRawInputDevices(&Mouse, 1, sizeof(RAWINPUTDEVICE))) {
        core::lastError::Description Dsc;
        X_ERROR("Mouse", "Failed to unregister mouse: %s", core::lastError::ToString(Dsc));
    }
}

///////////////////////////////////////////
void XMouse::Update(core::FrameData& frameData)
{
    X_ASSERT_UNREACHABLE();
    X_UNUSED(frameData);
}

void XMouse::ProcessInput(const uint8_t* pData, core::FrameInput& inputFrame)
{
    const RAWMOUSE& mouse = *reinterpret_cast<const RAWMOUSE*>(pData);
    ProcessMouseData(mouse, inputFrame);
}

void XMouse::PostEvent(InputSymbol* pSymbol, core::FrameInput& inputFrame)
{
    if (inputFrame.events.size() == inputFrame.events.capacity()) {
        X_WARNING("Mouse", "Exceeded input frame event limit of: %" PRIuS " dropping event for symbol: \"%s\"",
            inputFrame.events.size(), pSymbol->name.c_str());
        return;
    }

    pSymbol->AssignToEvent(inputFrame.events.AddOne(), GetIInput().GetModifiers());
}

void XMouse::PostOnlyIfChanged(InputSymbol* pSymbol, InputState::Enum newState, core::FrameInput& inputFrame)
{
    if (pSymbol->state != InputState::RELEASED && newState == InputState::RELEASED) {
        pSymbol->state = newState;
        pSymbol->value = 0.0f;
    }
    else if (pSymbol->state == InputState::RELEASED && newState == InputState::PRESSED) {
        pSymbol->state = newState;
        pSymbol->value = 1.0f;
    }
    else {
        return;
    }

    PostEvent(pSymbol, inputFrame);
}

void XMouse::OnButtonDown(KeyId::Enum id, core::FrameInput& inputFrame)
{
    InputSymbol* pSymbol = GetSymbol(id);

    pSymbol->value = 1.f;
    pSymbol->state = InputState::PRESSED;

    if (vars_.inputDebug_) {
        X_LOG0("Mouse", "Button Down: \"%s\"", pSymbol->name.c_str());
    }

    PostEvent(pSymbol, inputFrame);
}

void XMouse::OnButtonUP(KeyId::Enum id, core::FrameInput& inputFrame)
{
    InputSymbol* pSymbol = GetSymbol(id);

    pSymbol->value = 0.f;
    pSymbol->state = InputState::RELEASED;

    if (vars_.inputDebug_) {
        X_LOG0("Mouse", "Button Up: \"%s\"", pSymbol->name.c_str());
    }

    PostEvent(pSymbol, inputFrame);
}

void XMouse::ProcessMouseData(const RAWMOUSE& mouse, core::FrameInput& inputFrame)
{
    // i think i can get all info from raw input tbh.
    // get rekt.
    if (mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN) {
        OnButtonDown(KeyId::MOUSE_LEFT, inputFrame);
    }
    else if (mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_UP) {
        OnButtonUP(KeyId::MOUSE_LEFT, inputFrame);
    }

    // Right Mouse
    if (mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN) {
        OnButtonDown(KeyId::MOUSE_RIGHT, inputFrame);
    }
    else if (mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_UP) {
        OnButtonUP(KeyId::MOUSE_RIGHT, inputFrame);
    }

    // Middle Mouse
    if (mouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_DOWN) {
        OnButtonDown(KeyId::MOUSE_MIDDLE, inputFrame);
    }
    else if (mouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_UP) {
        OnButtonUP(KeyId::MOUSE_MIDDLE, inputFrame);
    }

    // aux 4
    if (mouse.usButtonFlags & RI_MOUSE_BUTTON_4_DOWN) {
        OnButtonDown(KeyId::MOUSE_AUX_4, inputFrame);
    }
    else if (mouse.usButtonFlags & RI_MOUSE_BUTTON_4_UP) {
        OnButtonUP(KeyId::MOUSE_AUX_4, inputFrame);
    }

    // aux 5
    if (mouse.usButtonFlags & RI_MOUSE_BUTTON_5_DOWN) {
        OnButtonDown(KeyId::MOUSE_AUX_5, inputFrame);
    }
    else if (mouse.usButtonFlags & RI_MOUSE_BUTTON_5_UP) {
        OnButtonUP(KeyId::MOUSE_AUX_5, inputFrame);
    }

    if (mouse.usButtonFlags & RI_MOUSE_WHEEL) {
        if (mouse.usFlags & MOUSE_MOVE_ABSOLUTE) {
            X_WARNING("Mouse", "Absolute scroll is not yet supported.");
        }

        mouseWheel_ = static_cast<float>(static_cast<short>(mouse.usButtonData));

        // we post scrool up and down once.
        InputState::Enum newState;

        // if the value is positive post a single up event.
        newState = (mouseWheel_ > 0.0f) ? InputState::PRESSED : InputState::RELEASED;

        if (vars_.inputDebug_ && newState == InputState::PRESSED) {
            X_LOG0("Mouse", "ScrollUp(%g)", mouseWheel_);
        }

        PostOnlyIfChanged(GetSymbol(KeyId::MOUSE_WHEELUP), newState, inputFrame);

        newState = (mouseWheel_ < 0.0f) ? InputState::PRESSED : InputState::RELEASED;

        if (vars_.inputDebug_ && newState == InputState::PRESSED) {
            X_LOG0("Mouse", "ScrollDown(%g)", mouseWheel_);
        }

        PostOnlyIfChanged(GetSymbol(KeyId::MOUSE_WHEELDOWN), newState, inputFrame);

        // we post scrool direction events.
        InputSymbol* pSymbol = GetSymbol(KeyId::MOUSE_Z);

        pSymbol->value = mouseWheel_;
        pSymbol->state = InputState::CHANGED;

        PostEvent(pSymbol, inputFrame);
    }

    if (mouse.usFlags == MOUSE_MOVE_RELATIVE) {
        if (mouse.lLastX != 0) {
            InputSymbol* pSymbol = GetSymbol(KeyId::MOUSE_X);

            pSymbol->value = (float)mouse.lLastX;
            pSymbol->state = InputState::CHANGED;

            if (vars_.inputMousePosDebug_) {
                X_LOG0("Mouse", "posX: \"%g\"", pSymbol->value);
            }

            PostEvent(pSymbol, inputFrame);
        }
        if (mouse.lLastY != 0) {
            InputSymbol* pSymbol = GetSymbol(KeyId::MOUSE_Y);

            pSymbol->value = (float)mouse.lLastY;
            pSymbol->state = InputState::CHANGED;

            if (vars_.inputMousePosDebug_) {
                X_LOG0("Mouse", "posY: \"%g\"", pSymbol->value);
            }

            PostEvent(pSymbol, inputFrame);
        }
    }

    // input_mouse_pos_debug
}

X_NAMESPACE_END