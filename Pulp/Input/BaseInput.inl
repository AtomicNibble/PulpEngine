#pragma once

X_NAMESPACE_BEGIN(input)

bool XBaseInput::Retriggering(void) const
{
    return retriggering_;
}

bool XBaseInput::IsEventPostingEnabled(void) const
{
    return enableEventPosting_;
}

bool XBaseInput::HasFocus(void) const
{
    return hasFocus_;
}

XBaseInput::ModifierFlags XBaseInput::GetModifiers(void)
{
    return modifiers_;
}

void XBaseInput::SetModifiers(ModifierFlags flags)
{
    this->modifiers_ = flags;
}

void XBaseInput::ClearModifiers(void)
{
    this->modifiers_.Clear();
}

InputSymbol* XBaseInput::DefineSymbol(InputDeviceType::Enum deviceType, KeyId::Enum id_,
    const KeyName& name_, InputSymbol::Type type_, ModifiersMasks::Enum mod_mask)
{
    InputSymbol* pSymbol = &InputSymbols_[id_];

    pSymbol->deviceType = deviceType;
    pSymbol->name = name_;
    pSymbol->type = type_;
    pSymbol->modifer_mask = mod_mask;
    pSymbol->keyId = id_;

    return pSymbol;
}

X_NAMESPACE_END
