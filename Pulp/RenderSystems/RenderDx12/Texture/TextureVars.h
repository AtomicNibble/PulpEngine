#pragma once

X_NAMESPACE_DECLARE(core,
                    struct ICVar;)

X_NAMESPACE_BEGIN(texture)

class TextureVars
{
public:
    TextureVars();
    ~TextureVars() = default;

    void registerVars(void);

private:
};

X_NAMESPACE_END

#include "TextureVars.inl"