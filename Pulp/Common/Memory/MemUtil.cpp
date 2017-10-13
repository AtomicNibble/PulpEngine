#include "EngineCommon.h"
#include "MemUtil.h"

X_NAMESPACE_BEGIN(core)

namespace Mem
{

// A faster version of memcopy that uses SSE instructions.  TODO:  Write an ARM variant if necessary.
void SIMDMemCopy(void* __restrict _Dest, const void* __restrict _Source, size_t NumQuadwords)
{
	X_ASSERT_ALIGNMENT(_Dest, 16, 0);
	X_ASSERT_ALIGNMENT(_Source, 16, 0);

	__m128i* __restrict pDest = (__m128i* __restrict)_Dest;
	const __m128i* __restrict pSource = (const __m128i* __restrict)_Source;

	// Discover how many quadwords precede a cache line boundary.  Copy them separately.
	size_t InitialQuadwordCount = (4 - ((size_t)pSource >> 4) & 3) & 3;
	if (InitialQuadwordCount > NumQuadwords) {
		InitialQuadwordCount = NumQuadwords;
	}

	switch (InitialQuadwordCount)
	{
	case 3: _mm_stream_si128(pDest + 2, _mm_load_si128(pSource + 2));	 // Fall through
	case 2: _mm_stream_si128(pDest + 1, _mm_load_si128(pSource + 1));	 // Fall through
	case 1: _mm_stream_si128(pDest + 0, _mm_load_si128(pSource + 0));	 // Fall through
	default:
		break;
	}

	if (NumQuadwords == InitialQuadwordCount) {
		return;
	}

	pDest += InitialQuadwordCount;
	pSource += InitialQuadwordCount;
	NumQuadwords -= InitialQuadwordCount;

	size_t CacheLines = NumQuadwords >> 2;

	switch (CacheLines)
	{
	default:
	case 10: _mm_prefetch((char*)(pSource + 36), _MM_HINT_NTA);	// Fall through
	case 9:  _mm_prefetch((char*)(pSource + 32), _MM_HINT_NTA);	// Fall through
	case 8:  _mm_prefetch((char*)(pSource + 28), _MM_HINT_NTA);	// Fall through
	case 7:  _mm_prefetch((char*)(pSource + 24), _MM_HINT_NTA);	// Fall through
	case 6:  _mm_prefetch((char*)(pSource + 20), _MM_HINT_NTA);	// Fall through
	case 5:  _mm_prefetch((char*)(pSource + 16), _MM_HINT_NTA);	// Fall through
	case 4:  _mm_prefetch((char*)(pSource + 12), _MM_HINT_NTA);	// Fall through
	case 3:  _mm_prefetch((char*)(pSource + 8), _MM_HINT_NTA);	// Fall through
	case 2:  _mm_prefetch((char*)(pSource + 4), _MM_HINT_NTA);	// Fall through
	case 1:  _mm_prefetch((char*)(pSource + 0), _MM_HINT_NTA);	// Fall through

																// Do four quadwords per loop to minimize stalls.
		for (size_t i = CacheLines; i > 0; --i)
		{
			// If this is a large copy, start prefetching future cache lines.  This also prefetches the
			// trailing quadwords that are not part of a whole cache line.
			if (i >= 10)
				_mm_prefetch((char*)(pSource + 40), _MM_HINT_NTA);

			_mm_stream_si128(pDest + 0, _mm_load_si128(pSource + 0));
			_mm_stream_si128(pDest + 1, _mm_load_si128(pSource + 1));
			_mm_stream_si128(pDest + 2, _mm_load_si128(pSource + 2));
			_mm_stream_si128(pDest + 3, _mm_load_si128(pSource + 3));

			pDest += 4;
			pSource += 4;
		}

	case 0:	// No whole cache lines to read
		break;
	}

	// Copy the remaining quadwords
	switch (NumQuadwords & 3)
	{
	case 3: _mm_stream_si128(pDest + 2, _mm_load_si128(pSource + 2));	 // Fall through
	case 2: _mm_stream_si128(pDest + 1, _mm_load_si128(pSource + 1));	 // Fall through
	case 1: _mm_stream_si128(pDest + 0, _mm_load_si128(pSource + 0));	 // Fall through
	default:
		break;
	}

	_mm_sfence();
}

void SIMDMemFill(void* __restrict _Dest, __m128 FillVector, size_t NumQuadwords)
{
	X_ASSERT_ALIGNMENT(_Dest, 16, 0);

	const __m128i Source = _mm_castps_si128(FillVector);
	__m128i* __restrict pDest = (__m128i* __restrict)_Dest;

	switch (((size_t)pDest >> 4) & 3)
	{
	case 1: _mm_stream_si128(pDest++, Source); --NumQuadwords;	 // Fall through
	case 2: _mm_stream_si128(pDest++, Source); --NumQuadwords;	 // Fall through
	case 3: _mm_stream_si128(pDest++, Source); --NumQuadwords;	 // Fall through
	default:
		break;
	}

	size_t WholeCacheLines = NumQuadwords >> 2;

	// Do four quadwords per loop to minimize stalls.
	while (WholeCacheLines--)
	{
		_mm_stream_si128(pDest++, Source);
		_mm_stream_si128(pDest++, Source);
		_mm_stream_si128(pDest++, Source);
		_mm_stream_si128(pDest++, Source);
	}

	// Copy the remaining quadwords
	switch (NumQuadwords & 3)
	{
	case 3: _mm_stream_si128(pDest++, Source);	 // Fall through
	case 2: _mm_stream_si128(pDest++, Source);	 // Fall through
	case 1: _mm_stream_si128(pDest++, Source);	 // Fall through
	default:
		break;
	}

	_mm_sfence();
}


} // namepsace Mem


X_NAMESPACE_END