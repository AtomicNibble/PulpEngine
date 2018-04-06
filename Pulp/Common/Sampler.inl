
X_NAMESPACE_BEGIN(render)

namespace shader
{
    X_INLINE void Sampler::setName(const core::string& name)
    {
        name_ = name;
    }

    X_INLINE void Sampler::setName(const char* pName)
    {
        name_ = pName;
    }

    X_INLINE const core::string& Sampler::getName(void) const
    {
        return name_;
    }

    X_INLINE int16_t Sampler::getBindPoint(void) const
    {
        return bindPoint_;
    }

    X_INLINE int16_t Sampler::getBindCount(void) const
    {
        return bindCount_;
    }

} // namespace shader

X_NAMESPACE_END
