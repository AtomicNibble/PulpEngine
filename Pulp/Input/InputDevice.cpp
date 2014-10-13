#include "stdafx.h"
#include "InputDevice.h"

X_NAMESPACE_BEGIN(input)


XInputDevice::XInputDevice(IInput& input, const char* deviceName)
: m_input(input)
, m_deviceName(deviceName)
, m_deviceId(InputDevice::UNKNOWN)
, m_enabled(true),
m_idToInfo(g_InputArena, 256)
{

}

XInputDevice::~XInputDevice()
{
	m_idToInfo.free();
}

void XInputDevice::Update(bool bFocus)
{

}

void XInputDevice::ClearKeyState()
{

}

void XInputDevice::Enable(bool enable)
{
	m_enabled = enable;
}

//////////////////////////////////////////////////////////////////////////
InputSymbol* XInputDevice::LookupSymbol(KeyId::Enum id) const
{
	return IdToSymbol(id);
}


InputSymbol* XInputDevice::IdToSymbol(KeyId::Enum id) const
{
	TIdToSymbolMap::const_iterator i = m_idToInfo.find(id);
	if (i != m_idToInfo.end())
		return (*i).second;
	else
		return 0;
}

X_NAMESPACE_END