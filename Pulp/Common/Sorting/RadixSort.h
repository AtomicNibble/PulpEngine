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
	X_INLINE void radix_sort_uint8_fast(const uint8_t* pInBegin, const uint8_t* pInEnd, core::Array<IndexType>& sortedIndexes)
	{
		const IndexType num = safe_static_cast<IndexType, uintptr_t>(pInEnd - pInBegin);
		size_t count[0x100] = {};
		IndexType* pBuckets[0x100];

		sortedIndexes.resize(num);

		for (IndexType i = 0; i < num; i++) {
			count[pInBegin[i] & 0xFF]++;
		}

		IndexType* pBucketDest = sortedIndexes.ptr();

		// can't be index type if in.size == index::max()
		for (uint16_t i = 0; i < 0x100; pBucketDest += count[i++]) {
			pBuckets[i] = pBucketDest;
		}

		for (IndexType i = 0; i < num; i++) {
			*pBuckets[pInBegin[i] & 0xFF]++ = i;
		}
	}


	// one for 16 bit :D 
	template<typename IndexType>
	X_INLINE void radix_sort_uint16_buf(const uint16_t* pInBegin, const uint16_t* pInEnd,
		core::Array<IndexType>& sortedIndexes, core::MemoryArenaBase* tempIndexArena)
	{
		const IndexType num = safe_static_cast<IndexType, uintptr_t>(pInEnd - pInBegin);
		size_t count[0x100] = {};
		IndexType* pBuckets[0x100];

		core::Array<IndexType> indexTemp(tempIndexArena);

		sortedIndexes.resize(num);
		indexTemp.resize(num);

		for (IndexType i = 0; i < num; i++) {
			count[pInBegin[i] & 0xFF]++;
		}

		IndexType* pBucketDest = sortedIndexes.ptr();

		for (uint16_t i = 0; i < 0x100; pBucketDest += count[i++]) {
			pBuckets[i] = pBucketDest;
		}

		for (IndexType i = 0; i < num; i++) {
			*pBuckets[pInBegin[i] & 0xFF]++ = i;
		}

		// Handle the upper 8 bits...
		pBucketDest = indexTemp.ptr();
		core::zero_object(count);

		for (IndexType i = 0; i < num; i++) {
			count[(pInBegin[i] >> 8) & 0xFF]++;
		}

		for (uint16_t i = 0; i < 0x100; pBucketDest += count[i++]) {
			pBuckets[i] = pBucketDest;
		}

		for (IndexType i = 0; i < num; i++) {
			const IndexType sortedIdx = sortedIndexes[i];
			*pBuckets[(pInBegin[sortedIdx] >> 8) & 0xFF]++ = sortedIdx;
		}

		sortedIndexes.swap(indexTemp);
	}


	template<typename IndexType>
	X_INLINE void radix_sort_uint32_buf(const uint32_t* pInBegin, const uint32_t* pInEnd,
		core::Array<IndexType>& sortedIndexes, core::MemoryArenaBase* tempIndexArena)
	{
		const IndexType num = safe_static_cast<IndexType, uintptr_t>(pInEnd - pInBegin);
		size_t count[0x100] = {};
		IndexType* pBuckets[0x100];

		core::Array<IndexType> indexTemp(tempIndexArena);

		sortedIndexes.resize(num);
		indexTemp.resize(num);

		for (IndexType i = 0; i < num; i++) {
			count[pInBegin[i] & 0xFF]++;
		}

		IndexType* pBucketDest = indexTemp.ptr();

		for (uint16_t i = 0; i < 0x100; pBucketDest += count[i++]) {
			pBuckets[i] = pBucketDest;
		}

		for (IndexType i = 0; i < num; i++) {
			*pBuckets[pInBegin[i] & 0xFF]++ = i;
		}

		pBucketDest = sortedIndexes.ptr();
		IndexType* pSrc = indexTemp.ptr();

		for (size_t shift = 8; shift < 32; shift += 8)
		{
			core::zero_object(count);

			for (IndexType i = 0; i < num; i++) {
				count[(pInBegin[i] >> shift) & 0xFF]++;
			}

			IndexType* pBucketDestStart = pBucketDest;
			for (uint16_t i = 0; i < 0x100; pBucketDest += count[i++]) {
				pBuckets[i] = pBucketDest;
			}
			pBucketDest = pBucketDestStart;


			for (IndexType i = 0; i < num; i++) {
				const IndexType sortedIdx = pSrc[i];
				*pBuckets[(pInBegin[sortedIdx] >> shift) & 0xFF]++ = sortedIdx;
			}


			core::Swap(pSrc, pBucketDest);
		}
	}

	template<typename IndexType>
	X_INLINE void radix_sort_uint64_buf(const uint64_t* pInBegin, const uint64_t* pInEnd,
		core::Array<IndexType>& sortedIndexes, core::MemoryArenaBase* tempIndexArena)
	{
		const IndexType num = safe_static_cast<IndexType, uintptr_t>(pInEnd - pInBegin);
		size_t count[0x100] = {};
		IndexType* pBuckets[0x100];

		core::Array<IndexType> indexTemp(tempIndexArena);

		sortedIndexes.resize(num);
		indexTemp.resize(num);

		for (IndexType i = 0; i < num; i++) {
			count[pInBegin[i] & 0xFF]++;
		}

		IndexType* pBucketDest = indexTemp.ptr();

		for (uint16_t i = 0; i < 0x100; pBucketDest += count[i++]) {
			pBuckets[i] = pBucketDest;
		}

		for (IndexType i = 0; i < num; i++) {
			*pBuckets[pInBegin[i] & 0xFF]++ = i;
		}

		pBucketDest = sortedIndexes.ptr();
		IndexType* pSrc = indexTemp.ptr();

		for (size_t shift = 8; shift < 64; shift += 8)
		{
			core::zero_object(count);

			for (IndexType i = 0; i < num; i++) {
				count[(pInBegin[i] >> shift) & 0xFF]++;
			}

			IndexType* pBucketDestStart = pBucketDest;
			for (uint16_t i = 0; i < 0x100; pBucketDest += count[i++]) {
				pBuckets[i] = pBucketDest;
			}
			pBucketDest = pBucketDestStart;


			for (IndexType i = 0; i < num; i++) {
				const IndexType sortedIdx = pSrc[i];
				*pBuckets[(pInBegin[sortedIdx] >> shift) & 0xFF]++ = sortedIdx;
			}

			core::Swap(pSrc, pBucketDest);
		}

	}


	template<typename IndexType>
	X_INLINE void radix_sort_buf(const core::Array<uint8_t>& in,
		core::Array<IndexType>& sortedIndexes)
	{
		radix_sort_uint8_fast<IndexType>(in.begin(), in.end(), sortedIndexes);
	}

	template<typename IndexType>
	X_INLINE void radix_sort_buf(const core::Array<uint16_t>& in,
		core::Array<IndexType>& sortedIndexes, core::MemoryArenaBase* tempIndexArena)
	{
		radix_sort_uint16_buf<IndexType>(in.begin(), in.end(), sortedIndexes, tempIndexArena);
	}

	template<typename IndexType>
	X_INLINE void radix_sort_buf(const core::Array<uint32_t>& in,
		core::Array<IndexType>& sortedIndexes, core::MemoryArenaBase* tempIndexArena)
	{
		radix_sort_uint32_buf<IndexType>(in.begin(), in.end(), sortedIndexes, tempIndexArena);
	}

	template<typename IndexType>
	X_INLINE void radix_sort_buf(const core::Array<uint64_t>& in,
		core::Array<IndexType>& sortedIndexes, core::MemoryArenaBase* tempIndexArena)
	{
		radix_sort_uint64_buf<IndexType>(in.begin(), in.end(), sortedIndexes, tempIndexArena);
	}


	template<typename IndexType>
	X_INLINE void radix_sort_buf(const uint8_t* pIn, const uint8_t* pEnd,
		core::Array<IndexType>& sortedIndexes)
	{
		radix_sort_uint8_fast(pIn, pEnd, sortedIndexes);
	}

	template<typename IndexType>
	X_INLINE void radix_sort_buf(const uint8_t* pIn, const uint8_t* pEnd,
		core::Array<IndexType>& sortedIndexes, core::MemoryArenaBase* tempIndexArena)
	{
		X_UNUSED(tempIndexArena);
		radix_sort_uint8_fast(pIn, pEnd, sortedIndexes);
	}

	template<typename IndexType>
	X_INLINE void radix_sort_buf(const uint16_t* pIn, const uint16_t* pEnd,
		core::Array<IndexType>& sortedIndexes, core::MemoryArenaBase* tempIndexArena)
	{
		radix_sort_uint16_buf(pIn, pEnd, sortedIndexes, tempIndexArena);
	}

	template<typename IndexType>
	X_INLINE void radix_sort_buf(const uint32_t* pIn, const uint32_t* pEnd,
		core::Array<IndexType>& sortedIndexes, core::MemoryArenaBase* tempIndexArena)
	{
		radix_sort_uint32_buf(pIn, pEnd, sortedIndexes, tempIndexArena);
	}


	template<typename IndexType>
	X_INLINE void radix_sort_buf(const uint64_t* pIn, const uint64_t* pEnd,
		core::Array<IndexType>& sortedIndexes, core::MemoryArenaBase* tempIndexArena)
	{
		radix_sort_uint64_buf(pIn, pEnd, sortedIndexes, tempIndexArena);
	}


	
	class RadixSort
	{
	public:
		RadixSort(core::MemoryArenaBase* arena);
		~RadixSort();

		RadixSort& sort(const uint32* pInput, size_t num);

		std::pair<const uint32_t*, size_t> getIndexes(void) const;

	private:
		void checkSize(size_t num);

	private:
		core::Array<uint32_t> histogram_;
		core::Array<uint32_t*> buckets_;

		core::Array<uint32_t> indexes_;
		core::Array<uint32_t> indexes2_;

		// pointers to above arrays that are swapped.
		// could just swap the arrays if i wanted.
		uint32_t* pIndexes_;
		uint32_t* pIndexes2_;

		bool ranksValid_;
		bool _pad[3];
	};


} // namespace Sorting

X_NAMESPACE_END