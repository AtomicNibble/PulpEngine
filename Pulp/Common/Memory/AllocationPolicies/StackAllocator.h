#pragma once

#ifndef X_STACKALLOCATOR_H
#define X_STACKALLOCATOR_H

#include "Memory/MemoryAllocatorStatistics.h"


X_NAMESPACE_BEGIN(core)

/// \ingroup Memory
/// \brief A class that implements an allocation policy for memory arenas.
/// \details This class implements the concepts of an allocation policy as expected by the MemoryArena class.
///
/// The allocation scheme employed by this class is that of a non-growing stack allocator. Allocation requests are
/// satisfied by returning individual regions of memory carved out of a pre-allocated block of memory. This block of
/// memory is supplied by the user, and thus supports both heap-based as well as stack-based allocations.
///
/// The following example illustrates both kinds of allocation:
/// \code
///   // 4096 bytes of memory on the stack
///   char memory[4096] = {};
///
///   // the stack allocator uses this stack memory to satisfy all allocation requests
///   core::StackAllocator allocator(memory, memory+4096);
///
///   // allocate 64 KB on the heap
///   core::HeapArea heapArea(65536);
///
///   // let the stack allocator use this 64 KB of memory on the heap
///   core::StackAllocator allocator(heapArea.GetStart(), heapArea.GetEnd());
/// \endcode
/// Allocating memory using the stack allocator is \b extremely fast, because the allocator only needs to offset a pointer
/// internally whenever an allocation is made. Allocations can be freed individually, but only in a stack-like fashion.
/// This means that if allocations are made in the order A, B and C, they need to be freed in the order C, B, and A.
///
/// Common usage scenarios for a stack allocator include per-level allocations which can easily be freed in reverse order.
///
/// \remark Statistics gathering can be enabled/disabled via the preprocessor option \ref X_ENABLE_MEMORY_ALLOCATOR_STATISTICS.
/// If statistics are disabled, the allocator will still return a valid MemoryAllocatorStatistics object which is
/// initialized to its default values.
/// \remark Checking for wrong usage of the stack allocator can be enabled/disabled via the preprocessor option
/// \ref X_ENABLE_STACK_ALLOCATOR_CHECK. Enabling the check causes a small memory overhead and performance hit for
/// each allocation, and should therefore be disabled in retail builds.
class StackAllocator
{
	struct BlockHeader
	{
#if X_ENABLE_STACK_ALLOCATOR_CHECK
		uint32_t AllocationID_;
#endif
		size_t allocationOffset_;
		size_t AllocationSize_;
	};

public:
	/// Constructs the allocator in the given memory region.
	StackAllocator(void* start, void* end);

	/// \brief Allocates raw memory.
	/// \remark The returned pointer will always adhere to the following alignment requirements: <tt>((ptr + offset) % alignment) == 0</tt>.
	void* allocate(size_t size, size_t alignment, size_t offset);

	/// Frees an allocation.
	void free(void* ptr);
	void free(void* ptr, size_t size);

	/// Returns the original allocation size for an allocation.
	inline size_t getSize(void* allocation) const;

	inline size_t usableSize(void* allocation) const;

	/// Returns statistics regarding the allocations made by the allocator.
	MemoryAllocatorStatistics getStatistics(void) const;

private:
	char* start_;
	char* end_;
	char* current_;

#if X_ENABLE_STACK_ALLOCATOR_CHECK
	uint32_t allocationID_;
#endif

#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
	MemoryAllocatorStatistics statistics_;
#endif
};

#include "StackAllocator.inl"

X_NAMESPACE_END


#endif
