#pragma once

#include "ITexture.h"

X_NAMESPACE_DECLARE(core,
    struct XFile)

X_NAMESPACE_BEGIN(render)

namespace shader
{
    class Texture
    {
    public:
        Texture() = default;
        Texture(const char* pName, int16_t bindPoint, int16_t bindCount, texture::TextureType::Enum type);
        Texture(core::string& name, int16_t bindPoint, int16_t bindCount, texture::TextureType::Enum type);

        X_INLINE void setName(const core::string& name);
        X_INLINE void setName(const char* pName);

        X_INLINE const core::string& getName(void) const;
        X_INLINE int16_t getBindPoint(void) const;
        X_INLINE int16_t getBindCount(void) const;
        X_INLINE texture::TextureType::Enum getType(void) const;

        bool SSave(core::XFile* pFile) const;
        bool SLoad(core::XFile* pFile);

    private:
        core::string name_;
        int16_t bindPoint_;
        int16_t bindCount_;
        texture::TextureType::Enum type_;
    };

} // namespace shader

X_NAMESPACE_END

#include "Texture.inl"
