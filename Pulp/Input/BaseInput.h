#pragma once

#ifndef _X_INPUT_BASE_H_
#define _X_INPUT_BASE_H_

#include <ICore.h>
#include <IFrameData.h>

#include <Containers\Array.h>
#include <Util\UniquePointer.h>

X_NAMESPACE_BEGIN(input)

struct IInputEventListner;
class XInputCVars;
class XInputDevice;

class XBaseInput : public IInput
    , public ICoreEventListener
{
    // listener functionality
protected:
    typedef core::UniquePointer<XInputCVars> XInputCVarsPtr;
    typedef core::UniquePointer<XInputDevice> XInputDevicePtr;
    typedef core::Array<XInputDevicePtr> InputDevicesArr;
    typedef core::Array<InputSymbol*> InputSymbolsArr;
    typedef core::ArrayGrowMultiply<InputEvent> InputEventArr;

    X_NO_ASSIGN(XBaseInput);
    X_NO_COPY(XBaseInput);

public:
    XBaseInput(core::MemoryArenaBase* arena);
    virtual ~XBaseInput() X_OVERRIDE;

    // IInput
    void registerVars(void) X_OVERRIDE;
    void registerCmds(void) X_OVERRIDE;

    bool init(void) X_OVERRIDE;
    void shutDown(void) X_OVERRIDE;
    void release(void) X_OVERRIDE;

    void update(core::FrameInput& inputFrame) X_OVERRIDE;
    void clearKeyState(void) X_OVERRIDE;

    // ~IInput

    bool addInputDevice(XInputDevicePtr pDevice);

    // ISystemEventListener
    void OnCoreEvent(const CoreEventData& ed) X_OVERRIDE;
    // ~ISystemEventListener

    X_INLINE ModifierFlags getModifiers(void) const;
    X_INLINE void setModifiers(ModifierFlags flags);
    X_INLINE void clearModifiers(void);

    X_INLINE InputSymbol* defineSymbol(InputDeviceType::Enum deviceType, KeyId::Enum id_,
        const KeyName& name_, InputSymbol::Type type_ = InputSymbol::Type::Button,
        ModifiersMasks::Enum modMask = ModifiersMasks::NONE);

protected:
    void addClearEvents(core::FrameInput& inputFrame);
    void addHoldEvents(core::FrameInput& inputFrame);

    void addEventToHoldSymbols(const InputEvent& event);
    void clearHoldEvent(const InputSymbol* pSymbol);

protected:
    core::MemoryArenaBase* arena_;
    InputSymbolsArr holdSymbols_;
    InputEventArr clearStateEvents_;

    // input device management
    InputDevicesArr devices_;

    // CVars
    XInputCVarsPtr pCVars_;

    ModifierFlags modifiers_; // caps ALT, SHIFT etc.

    InputSymbol InputSymbols_[input::KeyId::MOUSE_LAST];
};

X_NAMESPACE_END

#include "BaseInput.inl"

#endif // _X_INPUT_BASE_H_
