#pragma once

X_NAMESPACE_BEGIN(input)

XBaseInput::ModifierFlags XBaseInput::getModifiers(void) const
{
    return modifiers_;
}

void XBaseInput::setModifiers(ModifierFlags flags)
{
    modifiers_ = flags;
}

void XBaseInput::clearModifiers(void)
{
    modifiers_.Clear();
}

InputSymbol* XBaseInput::defineSymbol(InputDeviceType::Enum deviceType, KeyId::Enum id_,
    const KeyName& name_, InputSymbol::Type type_, ModifiersMasks::Enum modMask)
{
    InputSymbol* pSymbol = &InputSymbols_[id_];

    pSymbol->deviceType = deviceType;
    pSymbol->name = name_;
    pSymbol->type = type_;
    pSymbol->modifierMask = modMask;
    pSymbol->keyId = id_;

    return pSymbol;
}

X_NAMESPACE_END
