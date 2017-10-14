#pragma once
#ifndef X_LINEARALLOCATOR_H
#define X_LINEARALLOCATOR_H

#include "Memory/MemoryAllocatorStatistics.h"


X_NAMESPACE_BEGIN(core)

/// \ingroup Memory
/// \brief A class that implements an allocation policy for memory arenas.
/// \details This class implements the concepts of an allocation policy as expected by the MemoryArena class.
///
/// The allocation scheme employed by this class is that of a non-growing linear allocator. Allocation requests are
/// satisfied by returning individual regions of memory carved out of a pre-allocated block of memory. This block of
/// memory is supplied by the user, and thus supports both heap-based as well as stack-based allocations.
///
/// The following example illustrates both kinds of allocation:
/// \code
///   // 4096 bytes of memory on the stack
///   char memory[4096] = {};
///
///   // the linear allocator uses this stack memory to satisfy all allocation requests
///   core::LinearAllocator allocator(memory, memory+4096);
///
///   // allocate 64 KB on the heap
///   core::HeapArea heapArea(65536);
///
///   // let the linear allocator use this 64 KB of memory on the heap
///   core::LinearAllocator allocator(heapArea.GetStart(), heapArea.GetEnd());
/// \endcode
/// Allocating memory using the linear allocator is \b extremely fast, because the allocator only needs to offset a pointer
/// internally whenever an allocation is made. However, one disadvantage of a linear allocation scheme is that individual
/// allocations cannot be freed. Instead, all allocations must be freed at once.
///
/// Common usage scenarios for a linear allocator include temporary per-frame allocations (such as for e.g. building a
/// command-buffer for the GPU), or allocations that have application-lifetime.

class LinearAllocator
{
public:
	/// Constructs the allocator in the given memory region.
	LinearAllocator(void* start, void* end);

	/// \brief Allocates raw memory.
	/// \remark The returned pointer will always adhere to the following alignment requirements: <tt>((ptr + offset) % alignment) == 0</tt>.
	void* allocate(size_t size, size_t alignment, size_t offset);

	/// \brief Empty implementation.
	/// \remark Although this function is empty, it is expected by a MemoryArena.
	X_INLINE void free(void* ptr);
	X_INLINE void free(void* ptr, size_t size);

	/// Resets the allocator to the start of its memory region.
	X_INLINE void reset(void);

	/// Returns the original allocation size for an allocation.
	X_INLINE size_t getSize(void* allocation) const;

	X_INLINE size_t usableSize(void* ptr) const;

	/// Returns statistics regarding the allocations made by the allocator.
	MemoryAllocatorStatistics getStatistics(void) const;

private:
	char* start_;
	char* end_;
	char* current_;

#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
	MemoryAllocatorStatistics statistics_;
#endif
};

#include "LinearAllocator.inl"

X_NAMESPACE_END


#endif
