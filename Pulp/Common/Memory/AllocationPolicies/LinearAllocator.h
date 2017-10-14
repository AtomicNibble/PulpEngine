#pragma once
#ifndef X_LINEARALLOCATOR_H
#define X_LINEARALLOCATOR_H

#include "Memory/MemoryAllocatorStatistics.h"


X_NAMESPACE_BEGIN(core)

class LinearAllocator
{
public:
	LinearAllocator(void* start, void* end);

	void* allocate(size_t size, size_t alignment, size_t offset);

	X_INLINE void free(void* ptr);
	X_INLINE void free(void* ptr, size_t size);

	X_INLINE void reset(void);

	X_INLINE size_t getSize(void* allocation) const;
	X_INLINE size_t usableSize(void* ptr) const;

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
