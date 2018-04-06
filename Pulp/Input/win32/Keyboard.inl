#pragma once

X_NAMESPACE_BEGIN(input)

int32_t XKeyboard::GetDeviceIndex(void) const
{
    // only one device of this type
    return 0;
}

bool XKeyboard::IsOfDeviceType(InputDeviceType::Enum type) const
{
    return type == InputDeviceType::KEYBOARD;
}

X_NAMESPACE_END
