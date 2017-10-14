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
	/// \brief The header stored for each chunk of each growing pool.
	/// \details In order to keep track of the pool allocator that satisfied an allocation request, we store the
	/// allocator's index into the look-up table in a header. This header is accessed whenever an allocation needs
	/// to be freed.
	X_PACK_PUSH(1)
	struct ChunkHeader
	{
		uint16_t allocatorIndex_;
	};
	X_PACK_POP

	static_assert(sizeof(ChunkHeader) == 2, "Chunkheader size is incorrect");

public:
	/// Defines the maximum allocation size that is handled by the micro allocator.
	static const size_t MAX_ALLOCATION_SIZE = 256;

	/// \brief Constructs a micro allocator.
	/// \details The allocator will never grow a pool larger than \a maxSizeInBytesPerPool, and only grows each pool in
	/// \a growSize chunks. Each chunk will reserve \c sizeof(ChunkHeader) bytes at the end of a chunk in order to store the header data.
	GrowingMicroAllocator( uint32_t maxSizeInBytesPerPool, uint32_t growSize, size_t maxAlignment, size_t offset );

	/// \brief Allocates raw memory.
	/// \remark The returned pointer will always adhere to the following alignment requirements: <tt>((ptr + offset) % alignment) == 0</tt>.
	void* allocate( size_t size, size_t alignment, size_t offset );

	/// Frees an allocation.
	void free( void* ptr );
	void free(void* ptr, size_t size);

	/// Returns the size of an allocation.
	X_INLINE size_t getSize(void* ptr) const;

	/// Returns statistics regarding the allocations made by the allocator.
	MemoryAllocatorStatistics getStatistics(void) const;

	/// Returns whether a given allocation belongs to this allocator.
	X_INLINE bool containsAllocation(void* ptr) const;
	X_INLINE bool containsAllocation(void* ptr, size_t sizeIn) const;
	X_INLINE bool containsAllocation(void* ptr, size_t* pSizeOut) const;

private:
#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
	/// Updates the allocator's statistics.
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
