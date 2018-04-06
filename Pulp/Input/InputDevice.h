#pragma once

#ifndef _X_INPUTDEVICE_H_
#define _X_INPUTDEVICE_H_

#include <IInput.h>
#include <Containers\HashMap.h>

X_NAMESPACE_BEGIN(input)

class XInputCVars;

class XInputDevice : public IInputDevice
{
    typedef core::HashMap<KeyId::Enum, InputSymbol*> TIdToSymbolMap;

public:
    XInputDevice(IInput& input, XInputCVars& vars, const char* pDeviceName);
    virtual ~XInputDevice() X_OVERRIDE;

    // IInputDevice
    X_INLINE const char* GetDeviceName(void) const X_OVERRIDE;
    X_INLINE bool Init(void) X_OVERRIDE;
    X_INLINE void PostInit(void) X_OVERRIDE;
    X_INLINE void ShutDown(void) X_OVERRIDE;
    void Update(core::FrameData& frameData) X_OVERRIDE;
    void Enable(bool enable) X_OVERRIDE;
    X_INLINE bool IsEnabled(void) const X_OVERRIDE;
    void ClearKeyState(InputEventArr& clearEvents) X_OVERRIDE;
    InputSymbol* LookupSymbol(KeyId::Enum id) const X_OVERRIDE;
    // ~IInputDevice

protected:
    X_INLINE IInput& GetIInput(void) const;

    InputSymbol* IdToSymbol(KeyId::Enum id) const;

protected:
    InputDeviceType::Enum deviceType_;
    bool enabled_;

    XInputCVars& vars_;

private:
    IInput& input_;           // point to input system in use
    core::string deviceName_; // name of the device (used for input binding)
    TIdToSymbolMap idToInfo_;

private:
    X_NO_ASSIGN(XInputDevice);
};

X_NAMESPACE_END

#include "InputDevice.inl"

#endif // !_X_INPUTDEVICE_H_
