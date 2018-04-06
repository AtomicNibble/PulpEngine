#include "stdafx.h"
#include "Keyboard.h"
#include "Win32Input.h"

#include <Util\LastError.h>

#include "InputCVars.h"

X_NAMESPACE_BEGIN(input)

namespace
{
    uint8_t ToAscii(uint32_t vKeyCode, uint32_t scanCode, uint8_t KState[256])
    {
        uint16_t ascii[2] = {0, 0};
        int32_t result = ::ToAsciiEx(vKeyCode, scanCode, KState, ascii, 0, GetKeyboardLayout(0));

        if (result == 2) {
            return static_cast<uint8_t>(ascii[1] ? ascii[1] : (ascii[0] >> 8));
        }
        else if (result == 1) {
            return static_cast<uint8_t>(ascii[0]);
        }

        return 0;
    }
} // namespace

#define ASSCI_CACHE_TABLE 1

XKeyboard::XKeyboard(XWinInput& input, XInputCVars& vars) :
    XInputDeviceWin32(input, vars, "keyboard")
{
    deviceType_ = InputDeviceType::KEYBOARD;

    pSymbol_.fill(nullptr);
    VkeyCharCache_.fill(false);
}

XKeyboard::~XKeyboard()
{
}

///////////////////////////////////////////
bool XKeyboard::Init(void)
{
    RAWINPUTDEVICE Keyboard;
    Keyboard.hwndTarget = 0;
    Keyboard.usUsagePage = 0x01;
    Keyboard.usUsage = 0x06;
    Keyboard.dwFlags = RIDEV_NOLEGACY; // fuck legacy messages! aka WM_KEYDOWN

    bool res = true;

    if (!RegisterRawInputDevices(&Keyboard, 1, sizeof(RAWINPUTDEVICE))) {
        core::lastError::Description Dsc;
        X_ERROR("Keyboard", "Failed to register keyboard: %s", core::lastError::ToString(Dsc));
        res = false;
    }

    // register the wins.
    IInput& input = GetIInput();

#define ADD_DIGITS(idx, not_used) \
    pSymbol_[KeyId::DIGIT_##idx] = input.DefineSymbol(InputDeviceType::KEYBOARD, KeyId::DIGIT_##idx, #idx);

#define ADD_NUMPAD(idx, not_used) \
    pSymbol_[KeyId::NUMPAD_##idx] = input.DefineSymbol(InputDeviceType::KEYBOARD, KeyId::NUMPAD_##idx, "NUMPAD " #idx);

#define ADD_FUNCTION(idx, not_used) \
    pSymbol_[KeyId::F##idx] = input.DefineSymbol(InputDeviceType::KEYBOARD, KeyId::F##idx, "F" #idx);

#define DEFINE_SYM(id, name) \
    pSymbol_[id] = input.DefineSymbol(InputDeviceType::KEYBOARD, id, name);

#define DEFINE_SYM_TYPE(id, name, type, modifier) \
    pSymbol_[id] = input.DefineSymbol(InputDeviceType::KEYBOARD, id, name, type, modifier);

    pSymbol_[KeyId::BACKSPACE] = input.DefineSymbol(InputDeviceType::KEYBOARD, KeyId::BACKSPACE, "Backspace");
    pSymbol_[KeyId::TAB] = input.DefineSymbol(InputDeviceType::KEYBOARD, KeyId::TAB, "Tab");
    pSymbol_[KeyId::CLEAR] = input.DefineSymbol(InputDeviceType::KEYBOARD, KeyId::CLEAR, "Clear");
    pSymbol_[KeyId::ENTER] = input.DefineSymbol(InputDeviceType::KEYBOARD, KeyId::ENTER, "Enter");
    pSymbol_[KeyId::CAPSLOCK] = input.DefineSymbol(InputDeviceType::KEYBOARD, KeyId::CAPSLOCK, "Capslock", InputSymbol::Toggle, ModifiersMasks::CAPSLOCK);
    pSymbol_[KeyId::ESCAPE] = input.DefineSymbol(InputDeviceType::KEYBOARD, KeyId::ESCAPE, "Esc");
    pSymbol_[KeyId::SPACEBAR] = input.DefineSymbol(InputDeviceType::KEYBOARD, KeyId::SPACEBAR, "Spacebar");
    pSymbol_[KeyId::INSERT] = input.DefineSymbol(InputDeviceType::KEYBOARD, KeyId::INSERT, "Insert", InputSymbol::Toggle, ModifiersMasks::INSERT);
    pSymbol_[KeyId::DELETE] = input.DefineSymbol(InputDeviceType::KEYBOARD, KeyId::DELETE, "Delete");
    pSymbol_[KeyId::HOME] = input.DefineSymbol(InputDeviceType::KEYBOARD, KeyId::HOME, "Home");
    pSymbol_[KeyId::END] = input.DefineSymbol(InputDeviceType::KEYBOARD, KeyId::END, "End");
    pSymbol_[KeyId::PAGE_UP] = input.DefineSymbol(InputDeviceType::KEYBOARD, KeyId::PAGE_UP, "Page Up");
    pSymbol_[KeyId::PAGE_DOWN] = input.DefineSymbol(InputDeviceType::KEYBOARD, KeyId::PAGE_DOWN, "Pade Down");

    pSymbol_[KeyId::LEFT_ARROW] = input.DefineSymbol(InputDeviceType::KEYBOARD, KeyId::LEFT_ARROW, "Left");
    pSymbol_[KeyId::UP_ARROW] = input.DefineSymbol(InputDeviceType::KEYBOARD, KeyId::UP_ARROW, "Up");
    pSymbol_[KeyId::RIGHT_ARROW] = input.DefineSymbol(InputDeviceType::KEYBOARD, KeyId::RIGHT_ARROW, "Right");
    pSymbol_[KeyId::DOWN_ARROW] = input.DefineSymbol(InputDeviceType::KEYBOARD, KeyId::DOWN_ARROW, "Down");

    X_PP_REPEAT_10(ADD_DIGITS, 0)

    pSymbol_[KeyId::A] = input.DefineSymbol(InputDeviceType::KEYBOARD, KeyId::A, "A");
    pSymbol_[KeyId::B] = input.DefineSymbol(InputDeviceType::KEYBOARD, KeyId::B, "B");
    pSymbol_[KeyId::C] = input.DefineSymbol(InputDeviceType::KEYBOARD, KeyId::C, "C");
    pSymbol_[KeyId::D] = input.DefineSymbol(InputDeviceType::KEYBOARD, KeyId::D, "D");
    pSymbol_[KeyId::E] = input.DefineSymbol(InputDeviceType::KEYBOARD, KeyId::E, "E");
    pSymbol_[KeyId::F] = input.DefineSymbol(InputDeviceType::KEYBOARD, KeyId::F, "F");
    pSymbol_[KeyId::G] = input.DefineSymbol(InputDeviceType::KEYBOARD, KeyId::G, "G");
    pSymbol_[KeyId::H] = input.DefineSymbol(InputDeviceType::KEYBOARD, KeyId::H, "H");
    pSymbol_[KeyId::I] = input.DefineSymbol(InputDeviceType::KEYBOARD, KeyId::I, "I");
    pSymbol_[KeyId::J] = input.DefineSymbol(InputDeviceType::KEYBOARD, KeyId::J, "J");
    pSymbol_[KeyId::K] = input.DefineSymbol(InputDeviceType::KEYBOARD, KeyId::K, "K");
    pSymbol_[KeyId::L] = input.DefineSymbol(InputDeviceType::KEYBOARD, KeyId::L, "L");
    pSymbol_[KeyId::M] = input.DefineSymbol(InputDeviceType::KEYBOARD, KeyId::M, "M");
    pSymbol_[KeyId::N] = input.DefineSymbol(InputDeviceType::KEYBOARD, KeyId::N, "N");
    pSymbol_[KeyId::O] = input.DefineSymbol(InputDeviceType::KEYBOARD, KeyId::O, "O");
    pSymbol_[KeyId::P] = input.DefineSymbol(InputDeviceType::KEYBOARD, KeyId::P, "P");
    pSymbol_[KeyId::Q] = input.DefineSymbol(InputDeviceType::KEYBOARD, KeyId::Q, "Q");
    pSymbol_[KeyId::R] = input.DefineSymbol(InputDeviceType::KEYBOARD, KeyId::R, "R");
    pSymbol_[KeyId::S] = input.DefineSymbol(InputDeviceType::KEYBOARD, KeyId::S, "S");
    pSymbol_[KeyId::T] = input.DefineSymbol(InputDeviceType::KEYBOARD, KeyId::T, "T");
    pSymbol_[KeyId::U] = input.DefineSymbol(InputDeviceType::KEYBOARD, KeyId::U, "U");
    pSymbol_[KeyId::V] = input.DefineSymbol(InputDeviceType::KEYBOARD, KeyId::V, "V");
    pSymbol_[KeyId::W] = input.DefineSymbol(InputDeviceType::KEYBOARD, KeyId::W, "W");
    pSymbol_[KeyId::X] = input.DefineSymbol(InputDeviceType::KEYBOARD, KeyId::X, "X");
    pSymbol_[KeyId::Y] = input.DefineSymbol(InputDeviceType::KEYBOARD, KeyId::Y, "Y");
    pSymbol_[KeyId::Z] = input.DefineSymbol(InputDeviceType::KEYBOARD, KeyId::Z, "Z");

    X_PP_REPEAT_10(ADD_NUMPAD, 0)

    pSymbol_[KeyId::NUMPAD_MULTIPLY] = input.DefineSymbol(InputDeviceType::KEYBOARD, KeyId::NUMPAD_MULTIPLY, "NUM MULTIPLY");
    pSymbol_[KeyId::NUMPAD_ADD] = input.DefineSymbol(InputDeviceType::KEYBOARD, KeyId::NUMPAD_ADD, "NUM ADD");
    pSymbol_[KeyId::NUMPAD_SUBTRACT] = input.DefineSymbol(InputDeviceType::KEYBOARD, KeyId::NUMPAD_SUBTRACT, "NUM SUBSTRACT");
    pSymbol_[KeyId::NUMPAD_DECIMAL] = input.DefineSymbol(InputDeviceType::KEYBOARD, KeyId::NUMPAD_DECIMAL, "NUM DECIMAL");
    pSymbol_[KeyId::NUMPAD_DIVIDE] = input.DefineSymbol(InputDeviceType::KEYBOARD, KeyId::NUMPAD_DIVIDE, "NUM DIVIDE");
    pSymbol_[KeyId::NUMPAD_ENTER] = input.DefineSymbol(InputDeviceType::KEYBOARD, KeyId::NUMPAD_ENTER, "NUM ENTER");

    X_PP_REPEAT_24(ADD_FUNCTION, 24)

    DEFINE_SYM_TYPE(KeyId::NUM_LOCK, "NUM_LOCK", InputSymbol::Toggle, ModifiersMasks::NUMLOCK);
    DEFINE_SYM_TYPE(KeyId::SCROLL_LOCK, "SCROLL_LOCK", InputSymbol::Toggle, ModifiersMasks::SCROLLOCK);

    DEFINE_SYM_TYPE(KeyId::LEFT_SHIFT, "LEFT_SHIFT", InputSymbol::Button, ModifiersMasks::LSHIFT);
    DEFINE_SYM_TYPE(KeyId::RIGHT_SHIFT, "RIGHT_SHIFT", InputSymbol::Button, ModifiersMasks::RSHIFT);
    DEFINE_SYM_TYPE(KeyId::LEFT_CONTROL, "LEFT_CONTROL", InputSymbol::Button, ModifiersMasks::LCTRL);
    DEFINE_SYM_TYPE(KeyId::RIGHT_CONTROL, "RIGHT_CONTROL", InputSymbol::Button, ModifiersMasks::RCTRL);
    DEFINE_SYM_TYPE(KeyId::LEFT_ALT, "LEFT_ALT", InputSymbol::Button, ModifiersMasks::LALT);
    DEFINE_SYM_TYPE(KeyId::RIGHT_ALT, "RIGHT_ALT", InputSymbol::Button, ModifiersMasks::RALT);
    DEFINE_SYM_TYPE(KeyId::LEFT_WINDOWS, "LEFT_WINDOWS", InputSymbol::Button, ModifiersMasks::LWIN);
    DEFINE_SYM_TYPE(KeyId::RIGHT_WINDOWS, "RIGHT_WINDOWS", InputSymbol::Button, ModifiersMasks::RWIN);
    //	DEFINE_SYM_TYPE(KeyId::APPLICATION, "APPLICATION", InputSymbol::Button, ModifiersMasks::);
    DEFINE_SYM(KeyId::APPLICATION, "APPLICATION");
    DEFINE_SYM(KeyId::PRINT, "PRINT");
    DEFINE_SYM(KeyId::PAUSE, "PAUSE");
    DEFINE_SYM(KeyId::CANCEL, "CANCEL");

    DEFINE_SYM(KeyId::OEM_PLUS, "OEM_PLUS");
    DEFINE_SYM(KeyId::OEM_COMMA, "OEM_COMMA");
    DEFINE_SYM(KeyId::OEM_MINUS, "OEM_MINUS");
    DEFINE_SYM(KeyId::OEM_PERIOD, "OEM_PERIOD");
    DEFINE_SYM(KeyId::OEM_1, "OEM_1");
    DEFINE_SYM(KeyId::OEM_2, "OEM_2");
    DEFINE_SYM(KeyId::OEM_3, "OEM_3");
    DEFINE_SYM(KeyId::OEM_4, "OEM_4");
    DEFINE_SYM(KeyId::OEM_5, "OEM_5");
    DEFINE_SYM(KeyId::OEM_6, "OEM_6");
    DEFINE_SYM(KeyId::OEM_7, "OEM_7");
    DEFINE_SYM(KeyId::OEM_8, "OEM_8");

    // set the char keys.

    // A-Z
    memset(&VkeyCharCache_[KeyId::A], 1, (KeyId::Z - KeyId::A) + 1);
    memset(&VkeyCharCache_[KeyId::DIGIT_0], 1, (KeyId::DIGIT_9 - KeyId::DIGIT_0) + 1);

    memset(&VkeyCharCache_[KeyId::NUMPAD_0], 1, (KeyId::NUMPAD_9 - KeyId::NUMPAD_0) + 1);

    VkeyCharCache_[KeyId::SPACEBAR] = true;
    VkeyCharCache_[KeyId::OEM_1] = true;
    VkeyCharCache_[KeyId::OEM_2] = true;
    VkeyCharCache_[KeyId::OEM_3] = true;
    VkeyCharCache_[KeyId::OEM_4] = true;
    VkeyCharCache_[KeyId::OEM_5] = true;
    VkeyCharCache_[KeyId::OEM_6] = true;
    VkeyCharCache_[KeyId::OEM_7] = true;
    VkeyCharCache_[KeyId::OEM_8] = true;

    VkeyCharCache_[KeyId::OEM_PLUS] = true;
    VkeyCharCache_[KeyId::OEM_COMMA] = true;
    VkeyCharCache_[KeyId::OEM_MINUS] = true;
    VkeyCharCache_[KeyId::OEM_PERIOD] = true;

    // always enable shit out outside of numpad :|
    VkeyCharCache_[KeyId::NUMPAD_DIVIDE] = true;
    VkeyCharCache_[KeyId::NUMPAD_MULTIPLY] = true;
    VkeyCharCache_[KeyId::NUMPAD_SUBTRACT] = true;
    VkeyCharCache_[KeyId::NUMPAD_ADD] = true;
    VkeyCharCache_[KeyId::NUMPAD_DECIMAL] = true;

    // setup ascii cache
    initAsciiCache();

    return res;
}

void XKeyboard::initAsciiCache(void)
{
    // asciiCache_
    core::zero_object(asciiCache_);

    int k;
    uint8_t sKState[256] = {0};
    uint32_t scanCode;

    for (k = 0; k < 256; k++) {
        scanCode = MapVirtualKey(k, 0);

        // lower case
        asciiCache_[k].lower = ToAscii(k, scanCode, sKState);

        // upper case
        sKState[VK_SHIFT] = 0x80;
        asciiCache_[k].upper = ToAscii(k, scanCode, sKState);
        sKState[VK_SHIFT] = 0;

        // alternate
        sKState[VK_CONTROL] = 0x80;
        sKState[VK_MENU] = 0x80;
        sKState[VK_LCONTROL] = 0x80;
        sKState[VK_LMENU] = 0x80;
        asciiCache_[k].alternate = ToAscii(k, scanCode, sKState);
        sKState[VK_CONTROL] = 0x0;
        sKState[VK_MENU] = 0x0;
        sKState[VK_LCONTROL] = 0x0;
        sKState[VK_LMENU] = 0x0;

        // caps lock
        sKState[VK_CAPITAL] = 0x01;
        asciiCache_[k].caps = ToAscii(k, scanCode, sKState);
        sKState[VK_CAPITAL] = 0;
    }
}

void XKeyboard::ShutDown(void)
{
    RAWINPUTDEVICE Keyboard;
    Keyboard.hwndTarget = 0;
    Keyboard.usUsagePage = 0x1;
    Keyboard.usUsage = 0x6;
    Keyboard.dwFlags = RIDEV_REMOVE;

    if (!RegisterRawInputDevices(&Keyboard, 1, sizeof(RAWINPUTDEVICE))) {
        core::lastError::Description Dsc;
        X_ERROR("Keyboard", "Failed to unregister keyboard: %s", core::lastError::ToString(Dsc));
    }
}

bool XKeyboard::IsCHAR(const InputEvent& event)
{
#ifdef ASSCI_CACHE_TABLE
    KeyId::Enum Vkey = event.keyId;

    //  special case, numpad is only a input if numlock is active
    if (Vkey >= KeyId::NUMPAD_0 && Vkey <= KeyId::NUMPAD_9) {
        if (event.modifiers.IsSet(ModifiersMasks::NUMLOCK)) {
            return true;
        }
        return false;
    }

    return VkeyCharCache_[Vkey];
#else
    USHORT Vkey = event.keyId;

    // can cache this later.
    if (Vkey >= KeyId::A && Vkey <= KeyId::Z)
        return true;
    if (Vkey >= KeyId::DIGIT_0 && Vkey <= KeyId::DIGIT_9)
        return true;

    // OEM keys
    if (Vkey == KeyId::OEM_PLUS)
        return true;
    if (Vkey == KeyId::OEM_COMMA)
        return true;
    if (Vkey == KeyId::OEM_MINUS)
        return true;
    if (Vkey == KeyId::OEM_PERIOD)
        return true;
    if (Vkey == KeyId::OEM_1)
        return true;
    if (Vkey == KeyId::OEM_2)
        return true;
    if (Vkey == KeyId::OEM_3)
        return true;
    if (Vkey == KeyId::OEM_4)
        return true;
    if (Vkey == KeyId::OEM_5)
        return true;
    if (Vkey == KeyId::OEM_6)
        return true;
    if (Vkey == KeyId::OEM_7)
        return true;
    if (Vkey == KeyId::OEM_8)
        return true;

    if (Vkey == KeyId::SPACEBAR)
        return true;

    if (event.modifiers.IsSet(ModifiersMasks::NUMLOCK))
        if (Vkey >= KeyId::NUMPAD_0 && Vkey <= KeyId::NUMPAD_9)
            return true;
    return false;
#endif
}

char XKeyboard::Event2Char(const InputEvent& event)
{
    KeyId::Enum key = event.keyId;

    if (key >= 256) {
        return '\0';
    }

    if (event.modifiers.IsSet(ModifiersMasks::NUMLOCK)) {
        switch (key) {
            case KeyId::NUMPAD_0:
                return '0';
            case KeyId::NUMPAD_1:
                return '1';
            case KeyId::NUMPAD_2:
                return '2';
            case KeyId::NUMPAD_3:
                return '3';
            case KeyId::NUMPAD_4:
                return '4';
            case KeyId::NUMPAD_5:
                return '5';
            case KeyId::NUMPAD_6:
                return '6';
            case KeyId::NUMPAD_7:
                return '7';
            case KeyId::NUMPAD_8:
                return '8';
            case KeyId::NUMPAD_9:
                return '9';
            default:
                break;
        }
    }

    if (event.modifiers.IsSet(ModifiersMasks::CAPSLOCK)) {
        if (event.modifiers.IsSet(ModifiersMasks::Shift)) {
            return asciiCache_[key].alternate;
        }
        return asciiCache_[key].caps;
    }
    if (event.modifiers.IsSet(ModifiersMasks::Shift)) {
        return asciiCache_[key].upper;
    }
    return asciiCache_[key].lower;
}

///////////////////////////////////////////
void XKeyboard::Update(core::FrameData& frameData)
{
    X_ASSERT_UNREACHABLE();
    X_UNUSED(frameData);
}

void XKeyboard::ProcessInput(const uint8_t* pData, core::FrameInput& inputFrame)
{
    const RAWKEYBOARD& RawKb = *reinterpret_cast<const RAWKEYBOARD*>(pData);

    ProcessKeyboardData(RawKb, inputFrame);
}

void XKeyboard::ProcessKeyboardData(const RAWKEYBOARD& RawKb, core::FrameInput& inputFrame)
{
    IInput& input = GetIInput();

    USHORT virtualKey = RawKb.VKey;
    USHORT scanCode = RawKb.MakeCode; // it's name is makecode yet MSDN describes it as scan code lol.
    USHORT flags = RawKb.Flags;

    // Potato ?
    if (virtualKey == KEYBOARD_OVERRUN_MAKE_CODE) {
        if (vars_.inputDebug_) { // debug only?
            X_WARNING("Keyboard", "overrun occured");
        }
        return;
    }

    if (virtualKey == VK_SHIFT) {
        // correct left-hand / right-hand SHIFT
        virtualKey = safe_static_cast<USHORT, UINT>(MapVirtualKey(scanCode, MAPVK_VSC_TO_VK_EX));
    }
    else if (virtualKey == VK_NUMLOCK) {
        // correct PAUSE/BREAK and NUM LOCK silliness, and set the extended bit
        scanCode = safe_static_cast<USHORT, UINT>(MapVirtualKey(virtualKey, MAPVK_VK_TO_VSC) | 0x100);
    }

    // e0 and e1 are escape sequences used for certain special keys, such as PRINT and PAUSE/BREAK.
    // see http://www.win.tue.nl/~aeb/linux/kbd/scancodes-1.html
    const bool isE0 = ((flags & RI_KEY_E0) != 0);
    const bool isE1 = ((flags & RI_KEY_E1) != 0);
    //	const bool IsUp = ((flags & RI_KEY_BREAK) != 0);
    //	const bool IsDown = flags == RI_KEY_MAKE;
    const bool IsDown = (flags & RI_KEY_BREAK) == 0;

    if (isE1) {
        // for escaped sequences, turn the virtual key into the correct scan code using MapVirtualKey.
        // however, MapVirtualKey is unable to map VK_PAUSE (this is a known bug), hence we map that by hand.
        if (virtualKey == VK_PAUSE) {
            scanCode = 0x45;
        }
        else {
            scanCode = safe_static_cast<USHORT, UINT>(MapVirtualKey(virtualKey, MAPVK_VK_TO_VSC));
        }
    }

    switch (virtualKey) {
            // right-hand CONTROL and ALT have their e0 bit set
        case VK_CONTROL:
            if (isE0)
                virtualKey = KeyId::RIGHT_CONTROL;
            else
                virtualKey = KeyId::LEFT_CONTROL;
            break;

        case VK_MENU:
            if (isE0)
                virtualKey = KeyId::RIGHT_ALT;
            else
                virtualKey = KeyId::LEFT_ALT;
            break;

            // NUMPAD ENTER has its e0 bit set
        case VK_RETURN:
            if (isE0)
                virtualKey = KeyId::NUMPAD_ENTER;
            break;

            // the standard INSERT, DELETE, HOME, END, PRIOR and NEXT keys will always have their e0 bit set, but the
            // corresponding keys on the NUMPAD will not.
        case VK_INSERT:
            if (!isE0)
                virtualKey = KeyId::NUMPAD_0;
            break;

        case VK_DELETE:
            if (!isE0)
                virtualKey = KeyId::NUMPAD_DECIMAL;
            break;

        case VK_HOME:
            if (!isE0)
                virtualKey = KeyId::NUMPAD_7;
            break;

        case VK_END:
            if (!isE0)
                virtualKey = KeyId::NUMPAD_1;
            break;

        case VK_PRIOR:
            if (!isE0)
                virtualKey = KeyId::NUMPAD_9;
            break;

        case VK_NEXT:
            if (!isE0)
                virtualKey = KeyId::NUMPAD_3;
            break;

            // the standard arrow keys will always have their e0 bit set, but the
            // corresponding keys on the NUMPAD will not.
        case VK_LEFT:
            if (!isE0)
                virtualKey = KeyId::NUMPAD_4;
            break;

        case VK_RIGHT:
            if (!isE0)
                virtualKey = KeyId::NUMPAD_6;
            break;

        case VK_UP:
            if (!isE0)
                virtualKey = KeyId::NUMPAD_8;
            break;

        case VK_DOWN:
            if (!isE0)
                virtualKey = KeyId::NUMPAD_2;
            break;

            // NUMPAD 5 doesn't have its e0 bit set
        case VK_CLEAR:
            if (!isE0)
                virtualKey = KeyId::NUMPAD_5;
            break;
    }

    if (vars_.inputDebug_) {
        //	X_LOG0("Keyboard", "Key: %i, down: %i, flags: %i", virtualKey, (int)IsDown, flags);
    }

    // get the symbol
    InputSymbol* pSymbol = this->pSymbol_[virtualKey];

    if (pSymbol) {
        InputState::Enum newstate;

        // get the modifiers.
        IInput::ModifierFlags modifierFlags = input.GetModifiers();

        if (IsDown) {
            if (pSymbol->state == InputState::DOWN) {
                if (vars_.inputDebug_) {
                    X_LOG0("Keyboard", "Skipped (%s) state is already down. flags: %i, value: %f",
                        pSymbol->name.c_str(), flags, pSymbol->value);
                }
                return;
            }

            if (pSymbol->type == InputSymbol::Toggle) {
                if (modifierFlags.IsSet(pSymbol->modifer_mask))
                    modifierFlags.Remove(pSymbol->modifer_mask);
                else
                    modifierFlags.Set(pSymbol->modifer_mask);
            }
            else if (pSymbol->modifer_mask != ModifiersMasks::NONE) {
                modifierFlags.Set(pSymbol->modifer_mask);
            }

            newstate = InputState::PRESSED;
            pSymbol->value = 1.f;
        }
        else {
            if (pSymbol->modifer_mask != ModifiersMasks::NONE && pSymbol->type == InputSymbol::Button) {
                // this key is a modifer but is not togle type :)
                modifierFlags.Remove(pSymbol->modifer_mask);
            }

            newstate = InputState::RELEASED;
            pSymbol->value = 0.f;
        }

        input.SetModifiers(modifierFlags);

        if (newstate == pSymbol->state) {
            if (vars_.inputDebug_) {
                X_LOG0("Keyboard", "Skipped (%s) state has not changed: %s, flags: %i, value: %f",
                    pSymbol->name.c_str(), InputState::toString(newstate), modifierFlags.ToInt(), pSymbol->value);
            }
            return;
        }

        pSymbol->state = newstate;

        bool isChar = false;

        // make sure we have room for event plus possible char event.
        if (inputFrame.events.size() + 1 == inputFrame.events.capacity()) {
            X_WARNING("Keyboard", "Exceeded input frame event limit of: %" PRIuS " dropping event for symbol: \"%s\"",
                inputFrame.events.size(), pSymbol->name.c_str());
            return;
        }

        InputEvent& event = inputFrame.events.AddOne();
        event.deviceType = InputDeviceType::KEYBOARD;
        event.keyId = (KeyId::Enum)virtualKey;
        event.modifiers = modifierFlags;
        event.name = pSymbol->name;
        event.action = newstate;
        event.pSymbol = pSymbol;

        isChar = IsCHAR(event);

        if (isChar) {
            event.inputchar = Event2Char(event);
        }

        if (vars_.inputDebug_) {
            X_LOG0("Keyboard", "VK: %i, state: %s, name: %s", virtualKey, InputState::toString(newstate),
                event.name.c_str());
        }

        // if it's a char post it again.
        if (newstate == InputState::PRESSED && isChar) {
            InputEvent& charEvent = inputFrame.events.AddOne();
            charEvent = event;
            charEvent.action = InputState::CHAR;
            charEvent.value = 1.f;
        }
    }
    else if (vars_.inputDebug_) {
        X_WARNING("Keyboard", "failed to find symbol for VK: %i, SK: %i, Flags: %i",
            virtualKey, scanCode, flags);
    }
}

//////////////////////////////////////////////////////////////////////////
void XKeyboard::ClearKeyState(InputEventArr& clearEvents)
{
    IInput& input = GetIInput();

    // we need to clear the modifiers.
    IInput::ModifierFlags flags = input.GetModifiers();

    // clear any lock state flags.
    // flags &= ModifiersMasks::LockKeys;
    // flags.Remove(ModifiersMasks::LockKeys);
    flags.Remove(ModifiersMasks::SCROLLOCK);
    flags.Remove(ModifiersMasks::NUMLOCK);
    flags.Remove(ModifiersMasks::CAPSLOCK);

    bool bScroll = GetKeyState(VK_SCROLL) & 0x1;
    bool bNumLock = GetKeyState(VK_NUMLOCK) & 0x1;
    bool bCapsLock = GetKeyState(VK_CAPITAL) & 0x1;

    if (bScroll)
        flags.Set(ModifiersMasks::SCROLLOCK);
    if (bNumLock)
        flags.Set(ModifiersMasks::NUMLOCK);
    if (bCapsLock)
        flags.Set(ModifiersMasks::CAPSLOCK);

    input.SetModifiers(flags);

    // clear the symbols.
    for (size_t i = 0; i < 256; i++) {
        InputSymbol* pSymbol = pSymbol_[i];
        if (pSymbol) {
            if (pSymbol->value > 0.f) {
                // broadcast a release event and reset it etc.
                InputEvent& event = clearEvents.AddOne();
                event.deviceType = InputDeviceType::KEYBOARD;
                event.name = pSymbol->name;
                event.keyId = pSymbol->keyId;
                event.action = InputState::RELEASED;
                event.value = 0.0f;

                if (IsCHAR(event)) {
                    event.inputchar = Event2Char(event);
                    event.action = InputState::CHAR;
                }

                pSymbol->value = 0.0f;
                pSymbol->state = InputState::RELEASED;
            }
        }
    }

    if (vars_.inputDebug_) {
        X_LOG0("Keyboard", "keystates where cleared");
    }
}

X_NAMESPACE_END