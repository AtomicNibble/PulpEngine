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
    typedef core::UniquePointer<XInputDevice> XInputDevicePtr;
    typedef core::Array<XInputDevicePtr> InputDevicesArr;
    typedef core::Array<InputSymbol*> InputSymbolsArr;
    typedef core::Array<InputEvent> InputEventArr;

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

    bool job_PostInputFrame(core::V2::JobSystem& jobSys, core::FrameData& frameData) X_OVERRIDE;
    void update(core::FrameData& frameData) X_OVERRIDE;
    void clearKeyState(void) X_OVERRIDE;

    // ~IInput

    bool addInputDevice(XInputDevicePtr pDevice);

    // ISystemEventListener
    void OnCoreEvent(CoreEvent::Enum event, UINT_PTR wparam, UINT_PTR lparam) X_OVERRIDE;
    // ~ISystemEventListener

    X_INLINE ModifierFlags getModifiers(void);
    X_INLINE void setModifiers(ModifierFlags flags);
    X_INLINE void clearModifiers(void);

    X_INLINE InputSymbol* defineSymbol(InputDeviceType::Enum deviceType, KeyId::Enum id_,
        const KeyName& name_, InputSymbol::Type type_ = InputSymbol::Type::Button,
        ModifiersMasks::Enum modMask = ModifiersMasks::NONE);

protected:
    void AddClearEvents(core::FrameInput& inputFrame);
    void AddHoldEvents(core::FrameInput& inputFrame);

private:
    bool PostInputEvent(const InputEvent& event);
    void ClearHoldEvent(InputSymbol* pSymbol);
    bool SendEventToListeners(const InputEvent& event);
    void AddEventToHoldSymbols(const InputEvent& event);

protected:
    core::MemoryArenaBase* arena_;
    InputSymbolsArr holdSymbols_;
    InputEventArr clearStateEvents_;

    // input device management
    InputDevicesArr devices_;

    // CVars
    XInputCVars* pCVars_;

    ModifierFlags modifiers_; // caps ALT, SHIFT etc.

    InputSymbol InputSymbols_[input::KeyId::MOUSE_LAST];
};

X_NAMESPACE_END

#include "BaseInput.inl"

#endif // !_X_INPUT_BASE_H_
