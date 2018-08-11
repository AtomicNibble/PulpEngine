#pragma once

X_NAMESPACE_BEGIN(input)

const char* XInputDevice::getDeviceName(void) const
{
    return deviceName_.c_str();
}

X_NAMESPACE_END
