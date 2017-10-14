#pragma once
#ifndef X_GROWINGBLOCKALLOCATOR_H
#define X_GROWINGBLOCKALLOCATOR_H

#include "Memory/dlmalloc-2.8.6.h"
#include "Memory/MemoryAllocatorStatistics.h"


X_NAMESPACE_BEGIN(core)

/// \ingroup Memory
/// \brief A class that implements an allocation policy for memory arenas.
/// \details This class implements the concepts of an allocation policy as expected by the MemoryArena class.
///
/// The allocation scheme employed by this class is that of a general purpose allocator. Internally, the class uses
/// an implementation of \c dlmalloc. This allocator is part of Potato's custom generic allocator, and is mostly
/// intended to be used as an allocator for large blocks. In the context of the general purpose allocator, all allocations
/// which are greater than 256 bytes are considered to be "large", and all other allocations are normally being taken
/// care of by the micro allocator.
/// \remark Statistics gathering can be enabled/disabled via the preprocessor option \ref X_ENABLE_MEMORY_ALLOCATOR_STATISTICS.
/// If statistics are disabled, the allocator will still return a valid MemoryAllocatorStatistics object which is
/// initialized to its default values.
/// \remark Because the inner workings of \c dlmalloc can frequently change between versions, memory statistics will not
/// be as accurate compared to custom-built allocators.
class GrowingBlockAllocator
{
	/// \brief The header stored with each allocation.
	/// \details In order to enable custom alignment and keep track of an allocation's size, a header is stored next
	/// to each allocation.
	X_PACK_PUSH(4)
	struct BlockHeader
	{
		void* originalAllocation_;
		int32_t originalSize_;
	};
	X_PACK_POP;

public:
	/// Default constructor.
	GrowingBlockAllocator(void);

	/// Default destructor.
	~GrowingBlockAllocator(void);

	/// \brief Allocates raw memory.
	/// \remark The returned pointer will always adhere to the following alignment requirements: <tt>((ptr + offset) % alignment) == 0</tt>.
	void* allocate(size_t size, size_t alignment, size_t offset);

	/// Frees an allocation.
	void free(void* ptr);
	void free(void* ptr, size_t size);

	/// Returns the original allocation size for an allocation.
	X_INLINE size_t getSize(void* ptr) const;

	/// Returns statistics regarding the allocations made by the allocator.
	MemoryAllocatorStatistics getStatistics(void) const;

private:
	X_NO_COPY(GrowingBlockAllocator);
	X_NO_ASSIGN(GrowingBlockAllocator);


	mspace memorySpace_;

#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
	MemoryAllocatorStatistics statistics_;
#endif
};

#include "GrowingBlockAllocator.inl"

X_NAMESPACE_END


#endif
