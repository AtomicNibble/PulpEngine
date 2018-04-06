#pragma once

#ifndef _X_NULL_INPUT_H_
#define _X_NULL_INPUT_H_

#include <IInput.h>

X_NAMESPACE_BEGIN(input)

class XNullInput : public IInput
{
public:
    virtual void AddEventListener(IInputEventListner* pListener) X_OVERRIDE;
    virtual void RemoveEventListener(IInputEventListner* pListener) X_OVERRIDE;

    virtual void AddConsoleEventListener(IInputEventListner* pListener) X_OVERRIDE;
    virtual void RemoveConsoleEventListener(IInputEventListner* pListener) X_OVERRIDE;

    virtual bool AddInputDevice(IInputDevice* pDevice) X_OVERRIDE;

    virtual void EnableEventPosting(bool bEnable) X_OVERRIDE;
    virtual bool IsEventPostingEnabled(void) const X_OVERRIDE;
    virtual bool Job_PostInputFrame(core::V2::JobSystem& jobSys, core::FrameData& frameData) X_OVERRIDE;

    virtual bool Init(void) X_OVERRIDE;
    virtual void PostInit(void) X_OVERRIDE;
    virtual void ShutDown(void) X_OVERRIDE;
    virtual void release(void) X_OVERRIDE;
    virtual void Update(core::FrameData& frameData) X_OVERRIDE;

    virtual void registerVars(void) X_OVERRIDE;
    virtual void registerCmds(void) X_OVERRIDE;

    virtual void ClearKeyState(void) X_OVERRIDE;

    virtual void RetriggerKeyState(void) X_OVERRIDE;

    virtual bool Retriggering(void) const X_OVERRIDE;

    virtual bool HasInputDeviceOfType(InputDeviceType::Enum type) const X_OVERRIDE;

    virtual void EnableDevice(InputDeviceType::Enum deviceType, bool enable) X_OVERRIDE;

    virtual ModifierFlags GetModifiers(void) X_OVERRIDE;
    virtual void SetModifiers(ModifierFlags flags) X_OVERRIDE;

    virtual InputSymbol* DefineSymbol(InputDeviceType::Enum device,
        KeyId::Enum id_, const KeyName& name_,
        InputSymbol::Type type_ = InputSymbol::Type::Button,
        ModifiersMasks::Enum mod_mask = ModifiersMasks::NONE) X_OVERRIDE;
};

X_NAMESPACE_END

#endif // ! _X_NULL_INPUT_H_
