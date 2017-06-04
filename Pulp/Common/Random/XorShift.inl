
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

	X_INLINE uint32_t XorShift::randRange(uint32_t minValue, uint32_t maxValue)
	{
		X_ASSERT(minValue < maxValue, "Minimum value must be smaller than the maximum value.")(minValue, maxValue);
		const uint32_t range = maxValue - minValue;
		const uint32_t randomNumber = rand();
		return ((randomNumber % range) + minValue);
	}

	X_INLINE float XorShift::randRange(float minValue, float maxValue)
	{
		const float XOR_SHIFT_ONE_BY_MAX_UINT32 = 1.0f / 0xffffffff;

		X_ASSERT(minValue < maxValue, "Minimum value must be smaller than the maximum value.")(minValue, maxValue);
		const float range = maxValue - minValue;
		const uint32_t randomNumber = rand();
		return (static_cast<float>(randomNumber) * range * XOR_SHIFT_ONE_BY_MAX_UINT32) + minValue;
	}
}

X_NAMESPACE_END
