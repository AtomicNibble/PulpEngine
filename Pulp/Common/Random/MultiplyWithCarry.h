#pragma once

#ifndef X_MULTIPLY_WITH_CARRY_H_
#define X_MULTIPLY_WITH_CARRY_H_

X_NAMESPACE_BEGIN(core)

namespace random
{
    class MultiplyWithCarry
    {
    public:
        MultiplyWithCarry();
        explicit MultiplyWithCarry(const Vec4i& seed);

        void setSeed(const Vec4i& seed);

        X_INLINE uint32_t rand(void);
        X_INLINE size_t randIndex(size_t size);
        X_INLINE uint32_t randRange(uint32_t minValue, uint32_t maxValue);
        X_INLINE float randRange(float minValue, float maxValue);
        X_INLINE float randRange(float maxValue);

    private:
        uint32_t z_;
        uint32_t w_;
    };

} // namespace random

X_NAMESPACE_END

#include "MultiplyWithCarry.inl"

#endif // X_MULTIPLY_WITH_CARRY_H_
