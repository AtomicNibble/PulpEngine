#pragma once

X_NAMESPACE_BEGIN(level)

struct LvlVert
{
    Vec3f pos;
    Vec2f uv;
    Vec3f normal;
    Vec4<uint8> color;
};

struct LvlTris
{
    X_INLINE LvlTris()
    {
        pMaterial = nullptr;
    }

    engine::Material* pMaterial;
    LvlVert verts[3];
};

X_NAMESPACE_END
