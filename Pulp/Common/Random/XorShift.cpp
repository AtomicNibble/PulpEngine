#include "EngineCommon.h"
#include "XorShift.h"

X_NAMESPACE_BEGIN(core)

namespace random
{

	uint32_t xorShift_x = 123456789;
	uint32_t xorShift_y = 362436069;
	uint32_t xorShift_z = 521288629;
	uint32_t xorShift_w = 88675123;

//	const float XOR_SHIFT_ONE_BY_MAX_UINT32 = 1.0f / 0xffffffff;

	void XorShiftSeed(Vec4i& seed)
	{
		xorShift_x = seed[0];
		xorShift_y = seed[1];
		xorShift_z = seed[2];
		xorShift_w = seed[3];
	}
}


X_NAMESPACE_END