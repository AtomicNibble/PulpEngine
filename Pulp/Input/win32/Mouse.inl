#pragma once

X_NAMESPACE_BEGIN(input)

InputSymbol* XMouse::getSymbol(KeyId::Enum id)
{
    return pSymbol_[id - KeyId::INPUT_MOUSE_BASE];
}

X_NAMESPACE_END
