#pragma once

#ifndef X_XORSHIFT_H
#define X_XORSHIFT_H

X_NAMESPACE_BEGIN(core)

namespace random
{
    class XorShift
    {
    public:
        XorShift();
        explicit XorShift(const Vec4i& seed);

        void setSeed(const Vec4i& seed);

        X_INLINE uint32_t rand(void);
        X_INLINE size_t randIndex(size_t size);
        X_INLINE uint32_t randRange(uint32_t minValue, uint32_t maxValue);
        X_INLINE float randRange(float minValue, float maxValue);
        X_INLINE float randRange(float maxValue);

    private:
        Vec4i state_;
    };

    class XorShift128
    {
    public:
        XorShift128();
        explicit XorShift128(const Vec4i& seed);

        void setSeed(const Vec4i& seed);

        X_INLINE uint64_t rand(void);
        X_INLINE size_t randIndex(size_t size);
        X_INLINE uint64_t randRange(uint64_t minValue, uint64_t maxValue);
        X_INLINE float randRange(float minValue, float maxValue);
        X_INLINE float randRange(float maxValue);

    private:
        uint64_t state_[2];
    };
} // namespace random

X_NAMESPACE_END

#include "XorShift.inl"

#endif
