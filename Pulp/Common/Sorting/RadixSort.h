#pragma once


#include <Containers\Array.h>

X_NAMESPACE_BEGIN(core)

namespace Sorting
{

	// This returns a index list.
	// after sortedIndexes[0] contains the index in in[] for the lowest value.
	// 
	// Example:
	// 
	//	in[]:			43, 6, 12, 85, 35
	// 
	//	SortedIndex[]:	1,  2, 4,  0,  3
	// 
	template<typename IndexType>
	X_INLINE void radix_sort_uint8_fast(core::Array<uint8_t>& in, core::Array<IndexType>& sortedIndexes)
	{
		const IndexType num = safe_static_cast<IndexType, size_t>(in.size());
		size_t count[0x100] = {};
		IndexType* pBuckets[0x100];

		sortedIndexes.resize(num);

		for (IndexType i = 0; i < num; i++) {
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