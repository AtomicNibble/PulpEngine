#pragma once
#ifndef X_POOLALLOCATOR_H
#define X_POOLALLOCATOR_H

#include "Containers/Freelist.h"
#include "Memory/MemoryAllocatorStatistics.h"


X_NAMESPACE_BEGIN(core)

/// \ingroup Memory
/// \brief A class that implements an allocation policy for memory arenas.
/// \details This class implements the concepts of an allocation policy as expected by the MemoryArena class.
///
/// The allocation scheme employed by this class is that of a non-growing pool allocator. Allocation requests are
/// satisfied by returning individual regions of memory carved out of a pre-allocated block of memory. This block of
/// memory is supplied by the user, and thus supports both heap-based as well as stack-based allocations.
///
/// The following example illustrates both kinds of allocation:
/// \code
///   // 4096 bytes of memory on the stack
///   char memory[4096] = {};
///
///   // the pool allocator uses this stack memory to satisfy all allocation requests
///   core::PoolAllocator allocator(memory, memory+4096, 32, 16, 0);
///
///   // allocate 64 KB on the heap
///   core::HeapArea heapArea(65536);
///
///   // let the pool allocator use this 64 KB of memory on the heap
///   core::PoolAllocator allocator(heapArea.GetStart(), heapArea.GetEnd(), 32, 16, 0);
/// \endcode
/// The pool allocator uses a free list for managing allocated and freed slots internally. Both allocating and freeing
/// memory are \b extremely fast O(1) operations which only need to exchange a pointer in a linked-list. Furthermore,
/// in contrast to both the linear and the stack allocator, allocations can be made and freed in any order.
///
/// One of the disadvantages of a pool allocator is that it can only satisfy allocation requests of the same size, which
/// must be provided at the time the allocator is constructed. However, this implementation does allow making
/// allocations of a smaller size. Be aware that this can cause wasted memory, though.
///
/// Common usage scenarios for a pool allocator are situations in which lots of objects need to be allocated and freed
/// in random order, such as dynamic gameplay elements, or things like bullet holes, contact points, etc.
///
/// \remark Statistics gathering can be enabled/disabled via the preprocessor option \ref X_ENABLE_MEMORY_ALLOCATOR_STATISTICS.
/// If statistics are disabled, the allocator will still return a valid MemoryAllocatorStatistics object which is
/// initialized to its default values.
/// \remark Checking for wrong usage of the pool allocator can be enabled/disabled via the preprocessor option
/// \ref X_ENABLE_POOL_ALLOCATOR_CHECK. Enabling the check causes a small performance hit for
/// each allocation, and should therefore be disabled in retail builds.
class PoolAllocator
{
public:
	/// Constructs the allocator in the given memory region.
	PoolAllocator(void* start, void* end, size_t maxElementSize, size_t maxAlignment, size_t offset);

	/// \brief Allocates raw memory.
	/// \remark The returned pointer will always adhere to the following alignment requirements: <tt>((ptr + offset) % alignment) == 0</tt>.
	X_INLINE void* allocate(size_t size, size_t alignment, size_t offset);

	/// Frees an allocation.
	X_INLINE void free(void* ptr);
	X_INLINE void free(void* ptr, size_t size);

	/// Returns the original allocation size for an allocation.
	X_INLINE size_t getSize(void* allocation) const;

	X_INLINE size_t usableSize(void* ptr) const;

	/// Returns statistics regarding the allocations made by the allocator.
	MemoryAllocatorStatistics getStatistics(void) const;

private:
	Freelist freelist_;
	size_t maxSize_;

#if X_ENABLE_POOL_ALLOCATOR_CHECK
	size_t maxAlignment_;
	size_t offset_;
#endif

#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
	size_t elementSize_;
	size_t wastePerElement_;
	MemoryAllocatorStatistics statistics_;
#endif
};

#include "PoolAllocator.inl"

X_NAMESPACE_END


#endif
