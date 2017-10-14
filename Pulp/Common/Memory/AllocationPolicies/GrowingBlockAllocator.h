#pragma once
#ifndef X_GROWINGBLOCKALLOCATOR_H
#define X_GROWINGBLOCKALLOCATOR_H

#include "Memory/dlmalloc-2.8.6.h"
#include "Memory/MemoryAllocatorStatistics.h"


X_NAMESPACE_BEGIN(core)

class GrowingBlockAllocator
{
	X_PACK_PUSH(4)
	struct BlockHeader
	{
		void* originalAllocation_;
		int32_t originalSize_;
	};
	X_PACK_POP;

public:
	GrowingBlockAllocator(void);
	~GrowingBlockAllocator(void);

	void* allocate(size_t size, size_t alignment, size_t offset);

	void free(void* ptr);
	void free(void* ptr, size_t size);

	X_INLINE size_t getSize(void* ptr) const;
	size_t usableSize(void* ptr) const;

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
