#pragma once

#ifndef _X_INPUT_BASE_H_
#define _X_INPUT_BASE_H_

#include <ICore.h>
#include <IFrameData.h>

#include <Containers\Array.h>

X_NAMESPACE_BEGIN(input)

struct IInputEventListner;
class XInputCVars;

class XBaseInput : public IInput
    , public ICoreEventListener
{
    // listener functionality
    typedef core::Array<IInputDevice*> InputDevicesArr;
    typedef core::Array<IInputEventListner*> InputEventListenersList;
    typedef core::Array<InputSymbol*> InputSymbolsArr;
    typedef core::Array<InputEvent> InputEventArr;

public:
    XBaseInput(core::MemoryArenaBase* arena);
    virtual ~XBaseInput() X_OVERRIDE;

    // IInput
    void registerVars(void) X_OVERRIDE;
    void registerCmds(void) X_OVERRIDE;

    bool Init(void) X_OVERRIDE;
    void PostInit(void) X_OVERRIDE;
    void Update(core::FrameData& frameData) X_OVERRIDE;
    void ShutDown(void) X_OVERRIDE;
    void release(void) X_OVERRIDE;

    void ClearKeyState(void) X_OVERRIDE;
    void RetriggerKeyState(void) X_OVERRIDE;
    X_INLINE bool Retriggering(void) const X_OVERRIDE;
    bool HasInputDeviceOfType(InputDeviceType::Enum type) const X_OVERRIDE;
    bool AddInputDevice(IInputDevice* pDevice) X_OVERRIDE;
    void EnableEventPosting(bool bEnable) X_OVERRIDE;
    X_INLINE bool IsEventPostingEnabled(void) const X_OVERRIDE;
    bool Job_PostInputFrame(core::V2::JobSystem& jobSys, core::FrameData& frameData) X_OVERRIDE;

    // listener functions (implemented)
    void AddEventListener(IInputEventListner* pListener) X_OVERRIDE;
    void RemoveEventListener(IInputEventListner* pListener) X_OVERRIDE;
    void AddConsoleEventListener(IInputEventListner* pListener) X_OVERRIDE;
    void RemoveConsoleEventListener(IInputEventListner* pLstener) X_OVERRIDE;

    void EnableDevice(InputDeviceType::Enum deviceType, bool enable) X_OVERRIDE;
    // ~IInput

    // ISystemEventListener
    void OnCoreEvent(CoreEvent::Enum event, UINT_PTR wparam, UINT_PTR lparam) X_OVERRIDE;
    // ~ISystemEventListener

    X_INLINE bool HasFocus(void) const;

    X_INLINE ModifierFlags GetModifiers(void) X_OVERRIDE;
    X_INLINE void SetModifiers(ModifierFlags flags) X_OVERRIDE;
    X_INLINE void ClearModifiers(void);

    X_INLINE InputSymbol* DefineSymbol(InputDeviceType::Enum deviceType, KeyId::Enum id_,
        const KeyName& name_, InputSymbol::Type type_ = InputSymbol::Type::Button,
        ModifiersMasks::Enum mod_mask = ModifiersMasks::NONE) X_OVERRIDE;

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
    InputEventListenersList listners_;
    InputEventListenersList consoleListeners_;
    InputEventArr clearStateEvents_;

    // input device management
    InputDevicesArr devices_;

    // CVars
    XInputCVars* pCVars_;

    bool enableEventPosting_;
    bool retriggering_;
    bool hasFocus_;

    ModifierFlags modifiers_; // caps ALT, SHIFT etc.

    InputSymbol InputSymbols_[input::KeyId::MOUSE_LAST];

private:
    X_NO_ASSIGN(XBaseInput);
    X_NO_COPY(XBaseInput);
};

X_NAMESPACE_END

#include "BaseInput.inl"

#endif // !_X_INPUT_BASE_H_
