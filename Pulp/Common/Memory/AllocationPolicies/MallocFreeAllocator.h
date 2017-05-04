
#pragma once
#ifndef X_MALLOCFREEALLOCATOR_H
#define X_MALLOCFREEALLOCATOR_H

#include "Memory/MemoryAllocatorStatistics.h"


X_NAMESPACE_BEGIN(core)

/// \ingroup Memory
/// \brief A class that implements an allocation policy for memory arenas.
/// \details This class implements the concepts of an allocation policy as expected by the MemoryArena class.
///
/// The allocation scheme employed by this class is that of a general purpose allocator. Internally, the class uses
/// \c malloc and \c free provided by the C runtime. The implementation serves as a wrapper around \c malloc
/// and \c free which can be used in conjunction with the memory arena system. This enables users to use Potato
/// memory facilities like memory tracking and bounds checking with the platform's built-in allocator. Furthermore,
/// the allocator respects all alignment requirements, which is not supported by ordinary \c malloc.
/// \remark Statistics gathering can be enabled/disabled via the preprocessor option \ref X_ENABLE_MEMORY_ALLOCATOR_STATISTICS.
/// If statistics are disabled, the allocator will still return a valid MemoryAllocatorStatistics object which is
/// initialized to its default values.
/// \remark Because the inner workings of \c malloc and \c free are highly platform-dependent (and often undocumented),
/// memory statistics will not be as accurate compared to custom-built allocators.
class MallocFreeAllocator
{
	/// \brief The header stored with each allocation.
	/// \details In order to enable custom alignment and keep track of an allocation's size, a header is stored next
	/// to each allocation.
	struct BlockHeader
	{
		void* originalAllocation_;
		size_t AllocationSize_;
		size_t originalSize_;
	};


public:
	static const size_t SIZE_OF_HEADER = sizeof(BlockHeader);

public:
	/// Default constructor.
	MallocFreeAllocator(void);

	/// \brief Allocates raw memory.
	/// \remark The returned pointer will always adhere to the following alignment requirements: <tt>((ptr + offset) % alignment) == 0</tt>.
	void* allocate(size_t size, size_t alignment, size_t offset);

	/// Frees an allocation.
	void free(void* ptr);
	void free(void* ptr, size_t size);

	/// Returns the original allocation size for an allocation.
	inline size_t getSize(void* allocation) const;

	/// Returns statistics regarding the allocations made by the allocator.
	MemoryAllocatorStatistics getStatistics(void) const;

private:
#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
	MemoryAllocatorStatistics statistics_;
#endif
};

#include "MallocFreeAllocator.inl"

X_NAMESPACE_END


#endif
