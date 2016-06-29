#pragma once


#include <Containers\Array.h>

X_NAMESPACE_BEGIN(core)

namespace Sorting
{

	template<typename IndexType>
	X_INLINE void radix_sort_uint8_fast(core::Array<uint8_t>& in, core::Array<IndexType>& sortedIndexes)
	{
		const IndexType num = safe_static_cast<IndexType, size_t>(in.size());
		size_t count[0x100] = {};
		IndexType* pBuckets[0x100];

		sortedIndexes.resize(num);

		for (uint32_t i = 0; i < num; i++) {
			count[in[i] & 0xFF]++;
		}

		IndexType* pBucketDest = sortedIndexes.ptr();

		for (IndexType i = 0; i < 0x100; pBucketDest += count[i++]) {
			pBuckets[i] = pBucketDest;
		}

		for (IndexType i = 0; i < num; i++) {
			*pBuckets[in[i] & 0xFF]++ = i;
		}
	}

} // namespace Sorting

X_NAMESPACE_END