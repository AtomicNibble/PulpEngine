#pragma once

#ifndef _X_INPUTDEVICE_H_
#define _X_INPUTDEVICE_H_

#include <IInput.h>

//#include <string>
//#include <map>

#include <Containers\HashMap.h>

X_NAMESPACE_BEGIN(input)


class XInputDevice : public IInputDevice
{
	typedef core::HashMap<KeyId::Enum, InputSymbol*> TIdToSymbolMap;

public:
	XInputDevice(IInput& input, const char* deviceName);
	virtual ~XInputDevice() X_OVERRIDE;

	// IInputDevice
	virtual const char* GetDeviceName(void) const		X_OVERRIDE{ return deviceName_.c_str(); }
	virtual bool Init(void)	X_OVERRIDE{ return true; }
	virtual void PostInit(void) X_OVERRIDE{}
	virtual void ShutDown(void) X_OVERRIDE{}
	virtual void Update(bool focus) X_OVERRIDE;
	virtual void Enable(bool enable) X_OVERRIDE;
	virtual bool IsEnabled() const X_OVERRIDE{ return enabled_; }
	virtual void ClearKeyState(void) X_OVERRIDE;
	virtual bool SetExclusiveMode(bool value) X_OVERRIDE { X_UNUSED(value); return true; }
	virtual InputSymbol* LookupSymbol(KeyId::Enum id) const X_OVERRIDE;
	// ~IInputDevice

protected:
	X_INLINE IInput& GetIInput(void) const { return input_; }

	InputSymbol*				IdToSymbol(KeyId::Enum id) const;

protected:
	InputDeviceType::Enum		deviceType_;
	bool						enabled_;

private:
	IInput&						input_;		// point to input system in use
	core::string				deviceName_;	// name of the device (used for input binding)

private:
	TIdToSymbolMap				idToInfo_;

private:
	X_NO_ASSIGN(XInputDevice);
};



X_NAMESPACE_END

#endif // !_X_INPUTDEVICE_H_
