#pragma once
#ifndef X_POOLALLOCATOR_H
#define X_POOLALLOCATOR_H

#include "Containers/Freelist.h"
#include "Memory/MemoryAllocatorStatistics.h"


X_NAMESPACE_BEGIN(core)

class PoolAllocator
{
public:
	PoolAllocator(void* start, void* end, size_t maxElementSize, size_t maxAlignment, size_t offset);

	X_INLINE void* allocate(size_t size, size_t alignment, size_t offset);

	X_INLINE void free(void* ptr);
	X_INLINE void free(void* ptr, size_t size);

	X_INLINE size_t getSize(void* allocation) const;
	X_INLINE size_t usableSize(void* ptr) const;

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
