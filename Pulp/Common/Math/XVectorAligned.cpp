#include "EngineCommon.h"

#include "XVector.h"
#include "XVectorAligned.h"

// 1st math .cpp file O_O

namespace {


	__m128 dot_ps(register __m128 a, register __m128 b) 
	{
		// slower on AMD Athlon32, faster on Intel C2
		__m128 sq = _mm_mul_ps(a, b);
		sq.m128_f32[0] = sq.m128_f32[0]
			+ sq.m128_f32[1]
			+ sq.m128_f32[2]
			+ sq.m128_f32[3];
		return sq;
	}

	__m128 inv_sq_root_ss(register __m128 r)
	{
		// interation is: x1 = 0.5* x0 * (3 - R*x0^2)
		const float c3 = 3.0f;
		const float c05 = 0.5f;
		__m128 x0 = _mm_rsqrt_ss(r);
		__m128 x1 = _mm_mul_ss(x0, x0);
		x1 = _mm_mul_ss(x1, r);
		x1 = _mm_sub_ss(_mm_load_ss(&c3), x1);
		x1 = _mm_mul_ss(_mm_load_ss(&c05), x1);
		x1 = _mm_mul_ss(x1, x0);
		return x1;
	}

}



// the few functions we don't inline.

Vec4fA& Vec4fA::normalize()
{
	__m128 dot = dot_ps(sse, sse);
	__m128 isr = inv_sq_root_ss(dot);
	isr = _mm_shuffle_ps(isr, isr, 0x00);
	sse = _mm_mul_ps(sse, isr);
	return *this;
}


// Tests for zero-length
Vec4fA& Vec4fA::normalizeSafe()
{
	float s = lengthSquared();
	if (s > 0.f) {
		float invS = (1.f) / math<float>::sqrt(s);
		x *= invS;
		y *= invS;
		z *= invS;
		w = 0.f;
	}
	return *this;
}
