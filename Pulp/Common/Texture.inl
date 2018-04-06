
X_NAMESPACE_BEGIN(render)

namespace shader
{
    X_INLINE void Texture::setName(const core::string& name)
    {
        name_ = name;
    }

    X_INLINE void Texture::setName(const char* pName)
    {
        name_ = pName;
    }

    X_INLINE const core::string& Texture::getName(void) const
    {
        return name_;
    }

    X_INLINE int16_t Texture::getBindPoint(void) const
    {
        return bindPoint_;
    }

    X_INLINE int16_t Texture::getBindCount(void) const
    {
        return bindCount_;
    }

    X_INLINE texture::TextureType::Enum Texture::getType(void) const
    {
        return type_;
    }

} // namespace shader

X_NAMESPACE_END
