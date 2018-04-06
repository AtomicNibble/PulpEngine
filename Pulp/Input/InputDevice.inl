#pragma once

X_NAMESPACE_BEGIN(input)

const char* XInputDevice::GetDeviceName(void) const
{
    return deviceName_.c_str();
}

bool XInputDevice::Init(void)
{
    return true;
}

void XInputDevice::PostInit(void)
{
}

void XInputDevice::ShutDown(void)
{
}

bool XInputDevice::IsEnabled(void) const
{
    return enabled_;
}

IInput& XInputDevice::GetIInput(void) const
{
    return input_;
}

X_NAMESPACE_END
