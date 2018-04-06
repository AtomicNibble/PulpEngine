#pragma once

X_NAMESPACE_BEGIN(input)

int32_t XMouse::GetDeviceIndex(void) const
{
    // only one device of this type
    return 0;
}

bool XMouse::IsOfDeviceType(InputDeviceType::Enum type) const
{
    return type == InputDeviceType::MOUSE;
}

InputSymbol* XMouse::GetSymbol(KeyId::Enum id)
{
    return pSymbol_[id - KeyId::INPUT_MOUSE_BASE];
}

X_NAMESPACE_END
