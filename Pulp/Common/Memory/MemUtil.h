#pragma once



X_NAMESPACE_BEGIN(core)

namespace Mem
{

	void SIMDMemCopy(void* __restrict pDest, const void* __restrict pSource, size_t numQuadwords);
	void SIMDMemFill(void* __restrict pDest, __m128 FillVector, size_t numQuadwords);



} // namepsace Mem

X_NAMESPACE_END