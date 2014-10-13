#pragma once

#ifndef ME_XORSHIFT_H
#define ME_XORSHIFT_H


X_NAMESPACE_BEGIN(core)

namespace random
{
	inline uint32_t XorShift(void);
	inline uint32_t XorShift(uint32_t minValue, uint32_t maxValue);
	inline float XorShift(float minValue, float maxValue);
}

#include "XorShift.inl"

X_NAMESPACE_END


#endif
