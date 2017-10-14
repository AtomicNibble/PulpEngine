#pragma once

#ifndef X_GROWINGMICROALLOCATOR_H
#define X_GROWINGMICROALLOCATOR_H

#include "Memory/AllocationPolicies/GrowingPoolAllocator.h"
#include "Memory/MemoryAllocatorStatistics.h"


X_NAMESPACE_BEGIN(core)

// not using full lookup table is 1ns slower in micro benchmarks.
// but we don't have a 2kb lookup table to thrash the cache with.
#define X_USE_FULL_LOOKUP_TABLE 0

class GrowingMicroAllocator
{
	X_PACK_PUSH(1)
#if X_USE_FULL_LOOKUP_TABLE

	struct ChunkHeader
	{
		uint16_t allocatorIndex_;
	};

	static_assert(sizeof(ChunkHeader) == 2, "Chunkheader size is incorrect");

#else

	struct ChunkHeader
	{
		int8_t allocatorIndex_;
	};

	static_assert(sizeof(ChunkHeader) == 1, "Chunkheader size is incorrect");

#endif // !X_USE_FULL_LOOKUP_TABLE
	X_PACK_POP


public:
	static const size_t MAX_ALLOCATION_SIZE = 256;

public:
	GrowingMicroAllocator( uint32_t maxSizeInBytesPerPool, uint32_t growSize, size_t maxAlignment, size_t offset );

	void* allocate( size_t size, size_t alignment, size_t offset );

	void free( void* ptr );
	void free(void* ptr, size_t size);

	X_INLINE size_t getSize(void* ptr) const;
	X_INLINE size_t usableSize(void* ptr) const;

	MemoryAllocatorStatistics getStatistics(void) const;

	/// Returns whether a given allocation belongs to this allocator.
	X_INLINE bool containsAllocation(void* ptr) const;
	X_INLINE bool containsAllocation(void* ptr, size_t sizeIn) const;
	X_INLINE bool containsAllocation(void* ptr, size_t* pSizeOut) const;

private:
#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
	void updateStatistics(void);
#endif

	uint32_t chunkSize_;
	GrowingPoolAllocator poolAllocator8_;
	GrowingPoolAllocator poolAllocator16_;
	GrowingPoolAllocator poolAllocator32_;
	GrowingPoolAllocator poolAllocator64_;
	GrowingPoolAllocator poolAllocator128_;
	GrowingPoolAllocator poolAllocator256_;

#if X_USE_FULL_LOOKUP_TABLE
	GrowingPoolAllocator* poolAllocators_[257];
#else
	GrowingPoolAllocator* poolAllocators_[8];
#endif // !X_USE_FULL_LOOKUP_TABLE

#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
	MemoryAllocatorStatistics statistics_;
#endif
};

#include "GrowingMicroAllocator.inl"

X_NAMESPACE_END


#endif
