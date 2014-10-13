
namespace random
{

	X_INLINE uint32_t MultiplyWithCarry(void)
	{
		extern uint32_t mwc_z;
		extern uint32_t mwc_w;

		mwc_z = 36969u * (mwc_z & 65535u) + (mwc_z >> 16u);
		mwc_w = 18000u * (mwc_w & 65535u) + (mwc_w >> 16u);

		return (mwc_z << 16u) + (mwc_w & 65535u);
	}

	uint32_t MultiplyWithCarry(uint32_t minValue, uint32_t maxValue)
	{
		X_ASSERT(minValue < maxValue, "Minimum value must be smaller than the maximum value.")(minValue, maxValue);
		const uint32_t range = maxValue - minValue;
		const uint32_t randomNumber = MultiplyWithCarry();
		return ((randomNumber % range) + minValue);
	}

	float MultiplyWithCarry(float minValue, float maxValue)
	{
	//	extern const float MWC_ONE_BY_MAX_UINT32;
		const float MWC_ONE_BY_MAX_UINT32 = 1.f / UINT32_MAX;

		X_ASSERT(minValue < maxValue, "Minimum value must be smaller than the maximum value.")(minValue, maxValue);
		const float range = maxValue - minValue;
		const uint32_t randomNumber = MultiplyWithCarry();
		return (static_cast<float>(randomNumber) * range * MWC_ONE_BY_MAX_UINT32) + minValue;
	}
}
