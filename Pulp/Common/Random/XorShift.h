#pragma once

#ifndef ME_XORSHIFT_H
#define ME_XORSHIFT_H


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
		X_INLINE uint32_t randRange(uint32_t minValue, uint32_t maxValue);
		X_INLINE float randRange(float minValue, float maxValue);

	private:
		Vec4i state_;
	};
}

X_NAMESPACE_END

#include "XorShift.inl"


#endif
