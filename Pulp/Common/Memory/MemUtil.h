#pragma once



X_NAMESPACE_BEGIN(core)


void SIMDMemCopy(void* __restrict pDest, const void* __restrict pSource, size_t numQuadwords);
void SIMDMemFill(void* __restrict pDest, __m128 FillVector, size_t numQuadwords);


X_NAMESPACE_END