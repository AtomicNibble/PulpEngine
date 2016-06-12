#include "stdafx.h"
#include "InputDevice.h"

X_NAMESPACE_BEGIN(input)


XInputDevice::XInputDevice(IInput& input, const char* deviceName) :
	input_(input),
	deviceName_(deviceName),
	deviceType_(InputDeviceType::UNKNOWN),
	enabled_(true),
	idToInfo_(g_InputArena, 256)
{

}

XInputDevice::~XInputDevice()
{
	idToInfo_.free();
}

void XInputDevice::Update(core::FrameData& frameData)
{
	X_UNUSED(frameData);
}

void XInputDevice::ClearKeyState(InputEventArr& clearEvents)
{
	X_UNUSED(clearEvents);
}

void XInputDevice::Enable(bool enable)
{
	enabled_ = enable;
}

//////////////////////////////////////////////////////////////////////////
InputSymbol* XInputDevice::LookupSymbol(KeyId::Enum id) const
{
	return IdToSymbol(id);
}


InputSymbol* XInputDevice::IdToSymbol(KeyId::Enum id) const
{
	TIdToSymbolMap::const_iterator i = idToInfo_.find(id);
	if (i != idToInfo_.end()) {
		return (*i).second;
	}
	else {
		return 0;
	}
}

X_NAMESPACE_END