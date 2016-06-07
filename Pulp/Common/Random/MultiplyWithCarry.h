#pragma once

#ifndef ME_MULTIPLY_WITH_CARRY_H_
#define ME_MULTIPLY_WITH_CARRY_H_


X_NAMESPACE_BEGIN(core)

namespace random
{
	void MultiplyWithCarrySeed(Vec4i& seed);
	X_INLINE uint32_t MultiplyWithCarry(void);
	X_INLINE uint32_t MultiplyWithCarry(uint32_t minValue, uint32_t maxValue);
	X_INLINE float MultiplyWithCarry(float minValue, float maxValue);
}

#include "MultiplyWithCarry.inl"


X_NAMESPACE_END


#endif // ME_MULTIPLY_WITH_CARRY_H_
