
namespace random
{

	inline uint32_t XorShift(void)
	{
		extern uint32_t xorShift_x;
		extern uint32_t xorShift_y;
		extern uint32_t xorShift_z;
		extern uint32_t xorShift_w;

		const uint32_t t = xorShift_x ^ (xorShift_x << 11);
		xorShift_x = xorShift_y;
		xorShift_y = xorShift_z;
		xorShift_z = xorShift_w;
		xorShift_w ^= (xorShift_w >> 19) ^ (t ^ (t >> 8));

		return xorShift_w;
	}

	uint32_t XorShift(uint32_t minValue, uint32_t maxValue)
	{
		X_ASSERT(minValue < maxValue, "Minimum value must be smaller than the maximum value.")(minValue, maxValue);
		const uint32_t range = maxValue - minValue;
		const uint32_t randomNumber = XorShift();
		return ((randomNumber % range) + minValue);
	}

	float XorShift(float minValue, float maxValue)
	{
	//	extern const float XOR_SHIFT_ONE_BY_MAX_UINT32;
		const float XOR_SHIFT_ONE_BY_MAX_UINT32 = 1.0f / 0xffffffff;

		X_ASSERT(minValue < maxValue, "Minimum value must be smaller than the maximum value.")(minValue, maxValue);
		const float range = maxValue - minValue;
		const uint32_t randomNumber = XorShift();
		return (static_cast<float>(randomNumber) * range * XOR_SHIFT_ONE_BY_MAX_UINT32) + minValue;
	}
}
