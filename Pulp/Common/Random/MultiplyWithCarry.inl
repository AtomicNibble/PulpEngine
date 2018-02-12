
X_NAMESPACE_BEGIN(core)

namespace random
{

	X_INLINE uint32_t MultiplyWithCarry::rand(void)
	{
		z_ = 36969u * (z_ & 65535u) + (z_ >> 16u);
		w_ = 18000u * (w_ & 65535u) + (w_ >> 16u);
		return (z_ << 16u) + (w_ & 65535u);
	}

	X_INLINE size_t MultiplyWithCarry::randIndex(size_t size)
	{
		return rand() % size;
	}

	X_INLINE uint32_t MultiplyWithCarry::randRange(uint32_t minValue, uint32_t maxValue)
	{
		X_ASSERT(minValue < maxValue, "Minimum value must be smaller than the maximum value.")(minValue, maxValue);
		const uint32_t range = (maxValue - minValue) + 1;
		const uint32_t randomNumber = rand();
		return ((randomNumber % range) + minValue);
	}

	X_INLINE float MultiplyWithCarry::randRange(float minValue, float maxValue)
	{
		const float MWC_ONE_BY_MAX_UINT32 = 1.f / UINT32_MAX;

		X_ASSERT(minValue < maxValue, "Minimum value must be smaller than the maximum value.")(minValue, maxValue);
		const float range = (maxValue - minValue) + 1.f;
		const uint32_t randomNumber = rand();
		return (static_cast<float>(randomNumber) * range * MWC_ONE_BY_MAX_UINT32) + minValue;
	}

	X_INLINE float MultiplyWithCarry::randRange(float maxValue)
	{
		const float MWC_ONE_BY_MAX_UINT32 = 1.f / UINT32_MAX;

		X_ASSERT(maxValue > 0.f, "Maximum value must be greather than zero.")(maxValue);
		const uint32_t randomNumber = rand();
		return (static_cast<float>(randomNumber) * maxValue * MWC_ONE_BY_MAX_UINT32);
	}
}

X_NAMESPACE_END
