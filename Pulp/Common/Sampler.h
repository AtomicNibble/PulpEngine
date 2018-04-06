#pragma once

X_NAMESPACE_DECLARE(core,
    struct XFile)

X_NAMESPACE_BEGIN(render)

namespace shader
{
    class Sampler
    {
    public:
        Sampler() = default;
        Sampler(const char* pName, int16_t bindPoint, int16_t bindCount);
        Sampler(core::string& name, int16_t bindPoint, int16_t bindCount);

        X_INLINE void setName(const core::string& name);
        X_INLINE void setName(const char* pName);

        X_INLINE const core::string& getName(void) const;
        X_INLINE int16_t getBindPoint(void) const;
        X_INLINE int16_t getBindCount(void) const;

        bool SSave(core::XFile* pFile) const;
        bool SLoad(core::XFile* pFile);

    private:
        core::string name_;
        int16_t bindPoint_;
        int16_t bindCount_;
    };

} // namespace shader

X_NAMESPACE_END

#include "Sampler.inl"
