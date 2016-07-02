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

		// can't be index type if in.size == index::max()
		for (uint16_t i = 0; i < 0x100; pBucketDest += count[i++]) {
			pBuckets[i] = pBucketDest;
		}

		for (IndexType i = 0; i < num; i++) {
			*pBuckets[in[i] & 0xFF]++ = i;
		}
	}


	// one for 16 bit :D 
	template<typename IndexType>
	X_INLINE void radix_sort_uint16_buf(core::Array<uint16_t>& in, 
		core::Array<IndexType>& sortedIndexes, core::MemoryArenaBase* tempIndexArena)
	{
		const IndexType num = safe_static_cast<IndexType, size_t>(in.size());
		size_t count[0x100] = {};
		IndexType* pBuckets[0x100];

		core::Array<IndexType> indexTemp(tempIndexArena);

		sortedIndexes.resize(num);
		indexTemp.resize(num);

		for (IndexType i = 0; i < num; i++) {
			count[in[i] & 0xFF]++;
		}

		IndexType* pBucketDest = sortedIndexes.ptr();

		for (uint16_t i = 0; i < 0x100; pBucketDest += count[i++]) {
			pBuckets[i] = pBucketDest;
		}

		for (IndexType i = 0; i < num; i++) {
			*pBuckets[in[i] & 0xFF]++ = i;
		}

		// Handle the upper 8 bits...
		pBucketDest = indexTemp.ptr();
		core::zero_object(count);

		for (IndexType i = 0; i < num; i++) {
			count[(in[i] >> 8) & 0xFF]++;
		}

		for (uint16_t i = 0; i < 0x100; pBucketDest += count[i++]) {
			pBuckets[i] = pBucketDest;
		}

		for (IndexType i = 0; i < num; i++) {
			const IndexType sortedIdx = sortedIndexes[i];
			*pBuckets[(in[sortedIdx] >> 8) & 0xFF]++ = sortedIdx;
		}

		sortedIndexes.swap(indexTemp);
	}


	template<typename IndexType>
	X_INLINE void radix_sort_uint32_buf(core::Array<uint32_t>& in,
		core::Array<IndexType>& sortedIndexes, core::MemoryArenaBase* tempIndexArena)
	{
		const IndexType num = safe_static_cast<IndexType, size_t>(in.size());
		size_t count[0x100] = {};
		IndexType* pBuckets[0x100];

		core::Array<IndexType> indexTemp(tempIndexArena);

		sortedIndexes.resize(num);
		indexTemp.resize(num);

		for (IndexType i = 0; i < num; i++) {
			count[in[i] & 0xFF]++;
		}

		IndexType* pBucketDest = indexTemp.ptr();

		for (uint16_t i = 0; i < 0x100; pBucketDest += count[i++]) {
			pBuckets[i] = pBucketDest;
		}

		for (IndexType i = 0; i < num; i++) {
			*pBuckets[in[i] & 0xFF]++ = i;
		}

		pBucketDest = sortedIndexes.ptr();
		IndexType* pSrc = indexTemp.ptr();

		for (size_t shift = 8; shift < 32; shift += 8)
		{
			core::zero_object(count);

			for (IndexType i = 0; i < num; i++) {
				count[(in[i] >> shift) & 0xFF]++;
			}

			IndexType* pBucketDestStart = pBucketDest;
			for (uint16_t i = 0; i < 0x100; pBucketDest += count[i++]) {
				pBuckets[i] = pBucketDest;
			}
			pBucketDest = pBucketDestStart;


			for (IndexType i = 0; i < num; i++) {
				const IndexType sortedIdx = pSrc[i];
				*pBuckets[(in[sortedIdx] >> shift) & 0xFF]++ = sortedIdx;
			}


			core::Swap(pSrc, pBucketDest);
		}
	}

} // namespace Sorting

X_NAMESPACE_END