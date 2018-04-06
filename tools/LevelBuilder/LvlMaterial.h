#pragma once

X_NAMESPACE_BEGIN(level)

struct LvlMaterial
{
    LvlMaterial();

    const engine::MaterialFlags getFlags(void) const;

public:
    MaterialName name;
    Vec2f matRepeate;
    Vec2f shift;
    float rotate;
    engine::Material* pMaterial;
};

X_INLINE const engine::MaterialFlags LvlMaterial::getFlags(void) const
{
    X_ASSERT_NOT_NULL(pMaterial);
    return pMaterial->getFlags();
}

X_NAMESPACE_END