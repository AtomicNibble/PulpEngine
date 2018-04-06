#include "stdafx.h"
#include "InputDevice.h"

X_NAMESPACE_BEGIN(input)

XInputDevice::XInputDevice(IInput& input, XInputCVars& vars, const char* pDeviceName) :
    input_(input),
    vars_(vars),
    deviceName_(pDeviceName),
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
    auto it = idToInfo_.find(id);
    if (it != idToInfo_.end()) {
        return (*it).second;
    }

    return nullptr;
}

X_NAMESPACE_END