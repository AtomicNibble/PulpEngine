#pragma once

#ifndef X_GROWINGMICROALLOCATOR_H
#define X_GROWINGMICROALLOCATOR_H

#include "Memory/AllocationPolicies/GrowingPoolAllocator.h"
#include "Memory/MemoryAllocatorStatistics.h"


X_NAMESPACE_BEGIN(core)

/// \ingroup Memory
/// \brief A class that implements an allocation policy for memory arenas.
/// \details This class implements the concepts of an allocation policy as expected by the MemoryArena class.
///
/// The allocation scheme employed by this class is that of a growing micro allocator. The allocator is not restricted
/// to working within a fixed region of memory, but rather grows towards a predefined upper limit. Furthermore,
/// the micro allocator puts allocations of different sizes into different pools. The way this works is as follows:
/// - The micro allocator stores several pools, with each one being responsible for differently sized allocations.
/// - The pools hold power-of-two-sized allocations, hence the different pools are responsible for allocating 8, 16, 32,
/// 64, 128 and 256 bytes.
/// - Whenever an allocation request is made, the size of the allocation is rounded up to the next power-of-two, and
/// the corresponding pool is used to satisfy the request.
/// - In order to speed-up rounding and accessing the different pools, a small look-up table is used. This look-up table
/// can simply be indexed with an allocation's size, returning the correct pool.
/// - Whenever an allocation is freed, the allocator needs to know which pool was originally used to make the allocation.
/// This is achieved by storing a small header for each chunk in each growing pool, with the header simply storing the
/// index into the look-up table. Upon freeing an allocation, the chunk header can be accessed by rounding the
/// address to the chunk size, and subtracting the header's size. This works because all pools grow the same, according
/// to a specific size.
///
/// The allocator will initially only reserve virtual address space without allocating any physical memory. As the allocator needs to
/// grow, more and more physical memory is allocated, up to a given maximum size. Whenever an allocations is freed, the
/// allocator doesn't return physical memory to the OS.
///
/// Growing is implemented by making use of the virtual memory system. At first, the given amount of virtual address
/// space is reserved without allocating any physical memory. Whenever an allocation no longer fits into the currently
/// allocated physical memory, additional memory will be requested from the OS. The granularity with which the allocator
/// grows is provided by the user.
///
/// \remark Statistics gathering can be enabled/disabled via the preprocessor option \ref X_ENABLE_MEMORY_ALLOCATOR_STATISTICS.
/// If statistics are disabled, the allocator will still return a valid MemoryAllocatorStatistics object which is
/// initialized to its default values.
class GrowingMicroAllocator
{
	/// \brief The header stored for each chunk of each growing pool.
	/// \details In order to keep track of the pool allocator that satisfied an allocation request, we store the
	/// allocator's index into the look-up table in a header. This header is accessed whenever an allocation needs
	/// to be freed.
	//#pragma pack(push, 4)
	//X_ALIGNED_SYMBOL(struct,4)  ChunkHeader
	struct ChunkHeader
	{
		size_t allocatorIndex_;
	};
	//#pragma pack(pop)

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
	GrowingPoolAllocator* poolAllocators_[257];

#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
	MemoryAllocatorStatistics statistics_;
#endif
};

#include "GrowingMicroAllocator.inl"

X_NAMESPACE_END


#endif
