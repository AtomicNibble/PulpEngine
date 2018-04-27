#pragma once

#ifndef _X_INPUT_I_H_
#define _X_INPUT_I_H_

#include <Core\Platform.h>

#include <Util\Flags.h>
#include <Containers\Array.h>

// #include <Util\FlagsMacros.h>

#ifdef IPINPUT_EXPORTS
#define IPINPUT_API DLL_EXPORT
#else
#define IPINPUT_API DLL_IMPORT
#endif

struct ICore;

X_NAMESPACE_DECLARE(core,
                    struct FrameInput);

X_NAMESPACE_BEGIN(input)

static const size_t MAX_INPUT_EVENTS_PER_FRAME = 256;

// the mask's they set.
struct ModifiersMasks
{
    enum Enum
    {
        NONE = 0,
        LCTRL = BIT(0),
        LSHIFT = BIT(1),
        LALT = BIT(2),
        LWIN = BIT(3),
        RCTRL = BIT(4),
        RSHIFT = BIT(5),
        RALT = BIT(6),
        RWIN = BIT(7),

        NUMLOCK = BIT(8),
        CAPSLOCK = BIT(9),
        SCROLLOCK = BIT(10),
        INSERT = BIT(11),

        Ctrl = (LCTRL | RCTRL),
        Shift = (LSHIFT | RSHIFT),
        Alt = (LALT | RALT),
        Win = (LWIN | RWIN),
        Modifiers = (Ctrl | Shift | Alt | Win),
        LockKeys = (NUMLOCK | CAPSLOCK | SCROLLOCK)
    };

    struct Bits
    {
        uint32_t LCTRL : 1;
        uint32_t LSHIFT : 1;
        uint32_t LALT : 1;
        uint32_t LWIN : 1;
        uint32_t RCTRL : 1;
        uint32_t RSHIFT : 1;
        uint32_t RALT : 1;
        uint32_t RWIN : 1;

        uint32_t NUMLOCK : 1;
        uint32_t CAPSLOCK : 1;
        uint32_t SCROLLOCK : 1;
    };
};

struct InputState
{
    enum Enum
    {
        UNKNOWN,
        PRESSED = BIT(1),
        RELEASED = BIT(2),
        DOWN = BIT(3), // multiple pressed events elevate it to DOWN

        CHANGED = BIT(4), // mouse position changes etc.
        CHAR = BIT(5),
    };

    static const char* toString(InputState::Enum state)
    {
        switch (state) {
            case Enum::PRESSED:
                return "pressed";
            case Enum::RELEASED:
                return "released";
            case Enum::DOWN:
                return "down";
            case Enum::CHANGED:
                return "changed";
            case Enum::CHAR:
                return "char";
            case Enum::UNKNOWN:
                return "unknown";
#if X_DEBUG
            default:
                X_ASSERT_UNREACHABLE();
#else
                X_NO_SWITCH_DEFAULT;
#endif // !X_DEBUG
        }

        return "";
    }
};

X_DECLARE_ENUM(InputDeviceType)
(
    KEYBOARD,
    MOUSE,
    UNKNOWN);

// both keyboard and mouse map to this data
struct KeyId
{
    static const int INPUT_MOUSE_BASE = 0xff;

    typedef char Description[32];

    // this is keyboard and mouse keys mergerd
    // with mouse been offset.
    enum Enum
    {
        BACKSPACE = VK_BACK,
        TAB = VK_TAB,
        CLEAR = VK_CLEAR,
        ENTER = VK_RETURN,
        CAPSLOCK = VK_CAPITAL,
        ESCAPE = VK_ESCAPE,
        SPACEBAR = VK_SPACE,
        INSERT = VK_INSERT,
        DELETE = VK_DELETE,
        HOME = VK_HOME,
        END = VK_END,
        PAGE_UP = VK_PRIOR,
        PAGE_DOWN = VK_NEXT,

        // arrow keys
        LEFT_ARROW = VK_LEFT,
        UP_ARROW = VK_UP,
        RIGHT_ARROW = VK_RIGHT,
        DOWN_ARROW = VK_DOWN,

        // digits
        DIGIT_0 = 0x30,
        DIGIT_1 = 0x31,
        DIGIT_2 = 0x32,
        DIGIT_3 = 0x33,
        DIGIT_4 = 0x34,
        DIGIT_5 = 0x35,
        DIGIT_6 = 0x36,
        DIGIT_7 = 0x37,
        DIGIT_8 = 0x38,
        DIGIT_9 = 0x39,

        // letters
        A = 0x41,
        B = 0x42,
        C = 0x43,
        D = 0x44,
        E = 0x45,
        F = 0x46,
        G = 0x47,
        H = 0x48,
        I = 0x49,
        J = 0x4A,
        K = 0x4B,
        L = 0x4C,
        M = 0x4D,
        N = 0x4E,
        O = 0x4F,
        P = 0x50,
        Q = 0x51,
        R = 0x52,
        S = 0x53,
        T = 0x54,
        U = 0x55,
        V = 0x56,
        W = 0x57,
        X = 0x58,
        Y = 0x59,
        Z = 0x5A,

        // numpad keys
        NUMPAD_0 = VK_NUMPAD0,
        NUMPAD_1 = VK_NUMPAD1,
        NUMPAD_2 = VK_NUMPAD2,
        NUMPAD_3 = VK_NUMPAD3,
        NUMPAD_4 = VK_NUMPAD4,
        NUMPAD_5 = VK_NUMPAD5,
        NUMPAD_6 = VK_NUMPAD6,
        NUMPAD_7 = VK_NUMPAD7,
        NUMPAD_8 = VK_NUMPAD8,
        NUMPAD_9 = VK_NUMPAD9,
        NUMPAD_MULTIPLY = VK_MULTIPLY,
        NUMPAD_ADD = VK_ADD,
        NUMPAD_SUBTRACT = VK_SUBTRACT,
        NUMPAD_DECIMAL = VK_DECIMAL,
        NUMPAD_DIVIDE = VK_DIVIDE,
        NUMPAD_ENTER = VK_SEPARATOR,

        // F-keys
        F0 = 0xff, // needed so i can use macro.
        F1 = VK_F1,
        F2 = VK_F2,
        F3 = VK_F3,
        F4 = VK_F4,
        F5 = VK_F5,
        F6 = VK_F6,
        F7 = VK_F7,
        F8 = VK_F8,
        F9 = VK_F9,
        F10 = VK_F10,
        F11 = VK_F11,
        F12 = VK_F12,
        F13 = VK_F13,
        F14 = VK_F14,
        F15 = VK_F15,
        F16 = VK_F16,
        F17 = VK_F17,
        F18 = VK_F18,
        F19 = VK_F19,
        F20 = VK_F20,
        F21 = VK_F21,
        F22 = VK_F22,
        F23 = VK_F23,
        F24 = VK_F24,

        // lock keys
        NUM_LOCK = VK_NUMLOCK,
        SCROLL_LOCK = VK_SCROLL,

        // special keys
        LEFT_SHIFT = VK_LSHIFT,
        RIGHT_SHIFT = VK_RSHIFT,
        LEFT_CONTROL = VK_LCONTROL,
        RIGHT_CONTROL = VK_RCONTROL,
        LEFT_ALT = VK_LMENU,
        RIGHT_ALT = VK_RMENU,
        LEFT_WINDOWS = VK_LWIN,
        RIGHT_WINDOWS = VK_RWIN,
        APPLICATION = VK_APPS,
        PRINT = VK_SNAPSHOT,
        PAUSE = VK_PAUSE,
        CANCEL = VK_CANCEL,

        // OEM keys
        OEM_PLUS = VK_OEM_PLUS,
        OEM_COMMA = VK_OEM_COMMA,
        OEM_MINUS = VK_OEM_MINUS,
        OEM_PERIOD = VK_OEM_PERIOD,
        OEM_1 = VK_OEM_1,
        OEM_2 = VK_OEM_2,
        OEM_3 = VK_OEM_3,
        OEM_4 = VK_OEM_4,
        OEM_5 = VK_OEM_5,
        OEM_6 = VK_OEM_6,
        OEM_7 = VK_OEM_7,
        OEM_8 = VK_OEM_8,

        // Mouse
        MOUSE_LEFT = INPUT_MOUSE_BASE,
        MOUSE_MIDDLE,
        MOUSE_RIGHT,
        MOUSE_AUX_1,
        MOUSE_AUX_2,
        MOUSE_AUX_3,
        MOUSE_AUX_4,
        MOUSE_AUX_5,

        // These two are one shot events, use MOUSE_Z for repeating.
        MOUSE_WHEELUP,
        MOUSE_WHEELDOWN,

        // mouse movement sent as seprate events
        MOUSE_X,
        MOUSE_Y,
        MOUSE_Z, // wheel.

        MOUSE_LAST, // used for array size.

        UNKNOWN,
    };

    static const uint32_t ENUM_COUNT = UNKNOWN;
};

struct KeyName
{
    KeyName() :
        pName_(nullptr)
    {
    }

    KeyName(const char* pName) :
        pName_(pName)
    {
    }

    operator const char*() const
    {
        return pName_;
    }
    const char* c_str() const
    {
        return pName_;
    }

private:
    const char* pName_;
};

struct InputSymbol;

X_DISABLE_WARNING(4324);

X_ALIGNED_SYMBOL(struct InputEvent, 32)
{
    typedef Flags<ModifiersMasks> ModiferType;

    InputEvent()
    {
        deviceType = InputDeviceType::UNKNOWN;
        action = InputState::UNKNOWN;
        keyId = KeyId::UNKNOWN;
        modifiers = ModifiersMasks::NONE;
        value = 0.f;
        pSymbol = nullptr;
    }

    InputDeviceType::Enum deviceType; // keyboard, mouse etc
    InputState::Enum action;          //
    KeyId::Enum keyId;                // id for the event.
    ModiferType modifiers;            // Key modifiers enabled at the time of this event.
    KeyName name;                     // the name
    float value;                      // typically mouse pos.
    InputSymbol* pSymbol;             // Input symbol the event originated from.
    char inputchar;                   // pre translate
};

typedef core::FixedArray<InputEvent, MAX_INPUT_EVENTS_PER_FRAME> InputEventBuffer;

X_ENABLE_WARNING(4324);

struct InputSymbol
{
    enum Type
    {
        Button,
        Toggle
    };

    InputSymbol() :
        value(0.f),
        type(Button),
        keyId(KeyId::UNKNOWN),
        state(InputState::UNKNOWN)
    {
    }

    InputSymbol(const KeyName& name_, Type type_ = Type::Button) :
        //	keyid(id_),
        value(0.f),
        type(type_),
        name(name_)
    {
    }

    void AssignToEvent(InputEvent& event, InputEvent::ModiferType modifiers)
    {
        event.modifiers = modifiers;
        event.action = state;
        event.value = value;
        event.name = name;
        event.keyId = keyId;
        event.deviceType = deviceType;
    }

    float value;
    Type type;
    KeyName name;
    KeyId::Enum keyId;
    InputState::Enum state; // the state of the key, UP, DOWN
    InputDeviceType::Enum deviceType;
    ModifiersMasks::Enum modifer_mask; // if modifier has it's mask
};

// inherit from this if you want to register from input events.
struct IInputEventListner
{
    virtual ~IInputEventListner() = default;

    virtual bool OnInputEvent(const InputEvent& event) X_ABSTRACT;
    virtual bool OnInputEventChar(const InputEvent& event)
    {
        X_UNUSED(event);
        return false;
    };

    // higer priority is sent event first.
    virtual int32_t GetInputPriority(void) const
    {
        return 0;
    }
};

struct IInputDevice
{
    typedef core::Array<InputEvent> InputEventArr;

public:
    virtual ~IInputDevice() = default;

    virtual bool Init(void) X_ABSTRACT;
    virtual void PostInit(void) X_ABSTRACT;
    virtual void ShutDown(void) X_ABSTRACT;

    virtual const char* GetDeviceName(void) const X_ABSTRACT;
    virtual int GetDeviceIndex(void) const X_ABSTRACT;

    // Update.
    virtual void Update(core::FrameData& frameData) X_ABSTRACT;

    virtual void Enable(bool enable) X_ABSTRACT;
    virtual bool IsEnabled(void) const X_ABSTRACT;

    virtual void ClearKeyState(InputEventArr& clearEvents) X_ABSTRACT;

    virtual bool IsOfDeviceType(InputDeviceType::Enum type) const X_ABSTRACT;
    virtual InputSymbol* LookupSymbol(KeyId::Enum id) const X_ABSTRACT;
};

struct IInput
{
    typedef Flags<ModifiersMasks> ModifierFlags;

    virtual ~IInput() = default;

    // Registers new input events listener.
    virtual void AddEventListener(IInputEventListner* pListener) X_ABSTRACT;
    virtual void RemoveEventListener(IInputEventListner* pListener) X_ABSTRACT;

    // Registers new console input event listeners. console input listeners receive all events, no matter what.
    virtual void AddConsoleEventListener(IInputEventListner* pListener) X_ABSTRACT;
    virtual void RemoveConsoleEventListener(IInputEventListner* pListener) X_ABSTRACT;

    virtual bool AddInputDevice(IInputDevice* pDevice) X_ABSTRACT;

    virtual void EnableEventPosting(bool bEnable) X_ABSTRACT;
    virtual bool IsEventPostingEnabled(void) const X_ABSTRACT;
    virtual bool Job_PostInputFrame(core::V2::JobSystem& jobSys, core::FrameData& frameData) X_ABSTRACT;

    virtual void registerVars(void) X_ABSTRACT;
    virtual void registerCmds(void) X_ABSTRACT;

    virtual bool Init(void) X_ABSTRACT;
    virtual void PostInit(void) X_ABSTRACT;
    virtual void ShutDown(void) X_ABSTRACT;
    virtual void release(void) X_ABSTRACT;

    // called with frame data and parent job calling.
    virtual void Update(core::FrameData& frameData) X_ABSTRACT;

    virtual void ClearKeyState(void) X_ABSTRACT;

    // Re-triggers pressed keys.
    virtual void RetriggerKeyState(void) X_ABSTRACT;

    // true if currently re-triggering.
    virtual bool Retriggering(void) const X_ABSTRACT;

    //	Queries to see if this machine has some kind of input device connected.
    virtual bool HasInputDeviceOfType(InputDeviceType::Enum type) const X_ABSTRACT;

    // Tells devices whether to report input or not.
    virtual void EnableDevice(InputDeviceType::Enum type, bool enable) X_ABSTRACT;

    virtual ModifierFlags GetModifiers(void) X_ABSTRACT;
    virtual void SetModifiers(ModifierFlags flags) X_ABSTRACT;

    virtual InputSymbol* DefineSymbol(InputDeviceType::Enum deviceType, KeyId::Enum id_, const KeyName& name_,
        InputSymbol::Type type_ = InputSymbol::Type::Button, ModifiersMasks::Enum mod_mask = ModifiersMasks::NONE) X_ABSTRACT;
};

X_NAMESPACE_END

#ifdef __cplusplus
extern "C" {
#endif
typedef X_NAMESPACE(input)::IInput(*IP_PTRCREATEINPUTFNC(ICore* pCore, void* hwnd));

IPINPUT_API X_NAMESPACE(input)::IInput* CreateInput(ICore* pCore, void* hwnd);

#ifdef __cplusplus
};
#endif

#endif // !_X_INPUT_I_H_
