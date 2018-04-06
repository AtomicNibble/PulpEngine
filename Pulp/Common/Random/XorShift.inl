
X_NAMESPACE_BEGIN(core)

namespace random
{
    X_INLINE uint32_t XorShift::rand(void)
    {
        const uint32_t t = state_.x ^ (state_.x << 11);
        state_.x = state_.y;
        state_.y = state_.z;
        state_.z = state_.w;
        state_.w ^= (state_.w >> 19) ^ (t ^ (t >> 8));
        return state_.w;
    }

    X_INLINE size_t XorShift::randIndex(size_t size)
    {
        return rand() % size;
    }

    X_INLINE uint32_t XorShift::randRange(uint32_t minValue, uint32_t maxValue)
    {
        X_ASSERT(minValue < maxValue, "Minimum value must be smaller than the maximum value.")
        (minValue, maxValue);
        const uint32_t range = (maxValue - minValue) + 1;
        const uint32_t randomNumber = rand();
        return ((randomNumber % range) + minValue);
    }

    X_INLINE float XorShift::randRange(float minValue, float maxValue)
    {
        constexpr const float XOR_SHIFT_ONE_BY_MAX_UINT32 = 1.0f / std::numeric_limits<uint32_t>::max();

        X_ASSERT(minValue < maxValue, "Minimum value must be smaller than the maximum value.")
        (minValue, maxValue);
        const float range = (maxValue - minValue) + 1.f;
        const uint32_t randomNumber = rand();
        return (static_cast<float>(randomNumber) * range * XOR_SHIFT_ONE_BY_MAX_UINT32) + minValue;
    }

    X_INLINE float XorShift::randRange(float maxValue)
    {
        constexpr const float XOR_SHIFT_ONE_BY_MAX_UINT32 = 1.0f / std::numeric_limits<uint32_t>::max();

        X_ASSERT(maxValue > 0.f, "Maximum value must be greather than zero.")
        (maxValue);
        const uint32_t randomNumber = rand();
        return (static_cast<float>(randomNumber) * maxValue * XOR_SHIFT_ONE_BY_MAX_UINT32);
    }

    // ----------------------------------------

    X_INLINE uint64_t XorShift128::rand(void)
    {
        uint64_t a = state_[0];
        uint64_t b = state_[1];

        state_[0] = b;
        a ^= a << 23;
        a ^= a >> 18;
        a ^= b;
        a ^= b >> 5;
        state_[1] = a;

        return a + b;
    }

    X_INLINE size_t XorShift128::randIndex(size_t size)
    {
        return static_cast<size_t>(rand() % size);
    }

    X_INLINE uint64_t XorShift128::randRange(uint64_t minValue, uint64_t maxValue)
    {
        X_ASSERT(minValue < maxValue, "Minimum value must be smaller than the maximum value.")
        (minValue, maxValue);
        const uint64_t range = (maxValue - minValue) + 1;
        const uint64_t randomNumber = rand();
        return ((randomNumber % range) + minValue);
    }

    X_INLINE float XorShift128::randRange(float minValue, float maxValue)
    {
        const float XOR_SHIFT_ONE_BY_MAX = 1.0f / std::numeric_limits<uint64_t>::max();

        X_ASSERT(minValue < maxValue, "Minimum value must be smaller than the maximum value.")
        (minValue, maxValue);
        const float range = (maxValue - minValue) + 1.f;
        const uint64_t randomNumber = rand();
        return (static_cast<float>(randomNumber) * range * XOR_SHIFT_ONE_BY_MAX) + minValue;
    }

    X_INLINE float XorShift128::randRange(float maxValue)
    {
        const float XOR_SHIFT_ONE_BY_MAX_UINT32 = 1.0f / std::numeric_limits<uint64_t>::max();

        X_ASSERT(maxValue > 0.f, "Maximum value must be greather than zero.")
        (maxValue);
        const uint64_t randomNumber = rand();
        return (static_cast<float>(randomNumber) * maxValue * XOR_SHIFT_ONE_BY_MAX_UINT32);
    }

} // namespace random

X_NAMESPACE_END
