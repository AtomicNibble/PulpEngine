
#pragma once
#ifndef X_GROWINGSTACKALLOCATOR_H
#define X_GROWINGSTACKALLOCATOR_H

#include "Memory/MemoryAllocatorStatistics.h"


X_NAMESPACE_BEGIN(core)

/// \ingroup Memory
/// \brief A class that implements an allocation policy for memory arenas.
/// \details This class implements the concepts of an allocation policy as expected by the MemoryArena class.
///
/// The allocation scheme employed by this class is that of a growing stack allocator. The allocator behaves similar
/// to the non-growing StackAllocator, but is not restricted to working within a fixed region of memory. This allocator
/// will initially only reserve virtual address space without allocating any physical memory. As the allocator needs to
/// grow, more and more physical memory is allocated, up to a given maximum size. Whenever an allocations is freed, the
/// allocator doesn't return physical memory to the OS. This ensures that the allocator doesn't suffer from performance
/// penalties whenever some allocations/deallocations straddle a page boundary. Instead, physical memory which is no longer
/// needed must be explicitly released by calling Purge().
///
/// Growing is implemented by making use of the virtual memory system. At first, the given amount of virtual address
/// space is reserved without allocating any physical memory. Whenever an allocation no longer fits into the currently
/// allocated physical memory, additional memory will be requested from the OS. The granularity with which the allocator
/// grows is provided by the user, and the implementation automatically handles allocations that are larger than the
/// grow size.
///
/// \remark Statistics gathering can be enabled/disabled via the preprocessor option \ref X_ENABLE_MEMORY_ALLOCATOR_STATISTICS.
/// If statistics are disabled, the allocator will still return a valid MemoryAllocatorStatistics object which is
/// initialized to its default values.
/// \remark Checking for wrong usage of the stack allocator can be enabled/disabled via the preprocessor option
/// \ref X_ENABLE_STACK_ALLOCATOR_CHECK. Enabling the check causes a small memory overhead and performance hit for
/// each allocation, and should therefore be disabled in retail builds.
class GrowingStackAllocator
{
	X_PACK_PUSH(4)
	struct BlockHeader
	{
#if X_ENABLE_STACK_ALLOCATOR_CHECK
		uint32_t AllocationID_;
#endif
		uint32_t allocationOffset_;
		uint32_t allocationSize_;
	};
	X_PACK_POP;

public:
	/// \brief Constructs a stack allocator.
	/// \details The allocator will never grow larger than \a maxSizeInBytes, and only grows in \a growSize chunks.
	GrowingStackAllocator(size_t maxSizeInBytes, size_t granularity);

	/// Frees all physical memory owned by the allocator.
	~GrowingStackAllocator(void);

	/// \brief Allocates raw memory.
	/// \remark The returned pointer will always adhere to the following alignment requirements: <tt>((ptr + offset) % alignment) == 0</tt>.
	void* allocate(size_t size, size_t alignment, size_t offset);

	/// Frees an allocation.
	void free(void* ptr);
	void free(void* ptr, size_t size);

	/// Returns unused physical memory pages to the OS.
	void purge(void);

	/// Returns the original allocation size for an allocation.
	inline size_t getSize(void* allocation) const;
	
	inline size_t usableSize(void* ptr) const;

	/// Returns statistics regarding the allocations made by the allocator.
	MemoryAllocatorStatistics getStatistics(void) const;

private:
	char* virtualStart_;
	char* virtualEnd_;
	char* physicalCurrent_;
	char* physicalEnd_;
	size_t granularity_;

#if X_ENABLE_STACK_ALLOCATOR_CHECK
	uint32_t allocationID_;
#endif

#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
	MemoryAllocatorStatistics statistics_;
#endif
};

#include "GrowingStackAllocator.inl"

X_NAMESPACE_END


#endif
