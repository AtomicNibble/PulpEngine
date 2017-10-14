#pragma once
#ifndef X_GROWINGGENERICALLOCATOR_H
#define X_GROWINGGENERICALLOCATOR_H

#include "Memory/AllocationPolicies/GrowingMicroAllocator.h"
#include "Memory/AllocationPolicies/GrowingBlockAllocator.h"
#include "Memory/MemoryAllocatorStatistics.h"


X_NAMESPACE_BEGIN(core)

/// \ingroup Memory
/// \brief A class that implements an allocation policy for memory arenas.
/// \details This class implements the concepts of an allocation policy as expected by the MemoryArena class.
///
/// The allocation scheme employed by this class is that of a general purpose allocator. The allocator merges the
/// GrowingMicroAllocator and the GrowingBlockAllocator into one interface. All allocations which are smaller or equal
/// to GrowingMicroAllocator::MAX_ALLOCATION_SIZE are handled by the micro allocator, all other allocations are handled
/// by the block allocator. This ensures that allocations having a small size do not cause lots of overhead or wasted space, and can
/// be handled efficiently. In conjunction with the block allocator, this makes for a quite fast general purpose allocator.
/// \remark Statistics gathering can be enabled/disabled via the preprocessor option \ref X_ENABLE_MEMORY_ALLOCATOR_STATISTICS.
/// If statistics are disabled, the allocator will still return a valid MemoryAllocatorStatistics object which is
/// initialized to its default values.
class GrowingGenericAllocator
{
public:
	/// \brief Constructs a generic allocator.
	/// \details The allocator will never grow a pool of the micro allocator larger than \a maxSizeInBytesPerPool, and only grows each pool in
	/// \a growSize chunks.
	GrowingGenericAllocator( uint32_t maxSizeInBytesPerPool, uint32_t growSize, size_t maxAlignment, size_t offset);

	/// \brief Allocates raw memory.
	/// \remark The returned pointer will always adhere to the following alignment requirements: <tt>((ptr + offset) % alignment) == 0</tt>.
	X_INLINE void* allocate(size_t size, size_t alignment, size_t offset);

	/// Frees an allocation.
	X_INLINE void free(void* ptr);
	X_INLINE void free(void* ptr, size_t size);

	/// Returns the size of an allocation.
	X_INLINE size_t getSize(void* ptr) const;
	
	X_INLINE size_t usableSize(void* allocation) const;

	/// Returns statistics regarding the allocations made by the allocator.
	MemoryAllocatorStatistics getStatistics(void) const;
private:
#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
	void updateStatistics(void);
#endif

	X_NO_COPY(GrowingGenericAllocator);
	X_NO_ASSIGN(GrowingGenericAllocator);

	GrowingMicroAllocator microAllocator_;
	GrowingBlockAllocator blockAllocator_;

#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
	MemoryAllocatorStatistics statistics_;
#endif
};

#include "GrowingGenericAllocator.inl"

X_NAMESPACE_END


#endif
