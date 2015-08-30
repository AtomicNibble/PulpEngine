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
public:
	XInputDevice(IInput& input, const char* deviceName);
	virtual ~XInputDevice() X_OVERRIDE;

	// IInputDevice
	virtual const char* GetDeviceName() const		X_OVERRIDE{ return m_deviceName.c_str(); }
	virtual InputDevice::Enum GetDeviceId() const	X_OVERRIDE{ return m_deviceId; };
	virtual bool Init()	X_OVERRIDE{ return true; }
	virtual void PostInit() X_OVERRIDE{}
	virtual void ShutDown() X_OVERRIDE{}
	virtual void Update(bool bFocus) X_OVERRIDE;
	virtual void Enable(bool enable) X_OVERRIDE;
	virtual bool IsEnabled() const X_OVERRIDE{ return m_enabled; }
	virtual void ClearKeyState() X_OVERRIDE;
	virtual bool SetExclusiveMode(bool value) X_OVERRIDE{ return true; }
	virtual InputSymbol* LookupSymbol(KeyId::Enum id) const X_OVERRIDE;
	// ~IInputDevice

protected:
	IInput& GetIInput() const	{ return m_input; }

	InputSymbol*				IdToSymbol(KeyId::Enum id) const;

protected:
	InputDevice::Enum			m_deviceId;
	bool						m_enabled;
private:
	IInput&						m_input;		// point to input system in use
	core::string				m_deviceName;	// name of the device (used for input binding)

	typedef core::HashMap<KeyId::Enum, InputSymbol*>		TIdToSymbolMap;

	TIdToSymbolMap				m_idToInfo;
private:
	X_NO_ASSIGN(XInputDevice);
};



X_NAMESPACE_END

#endif // !_X_INPUTDEVICE_H_
