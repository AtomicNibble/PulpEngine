#pragma once

#ifndef X_GROWINGPOOLALLOCATOR_H
#define X_GROWINGPOOLALLOCATOR_H

#include "Containers/Freelist.h"
#include "Util/PointerUtil.h"
#include "Memory/MemoryAllocatorStatistics.h"

X_NAMESPACE_BEGIN(core)

class GrowingPoolAllocator
{
public:
    GrowingPoolAllocator(size_t maxSizeInBytes, size_t growSize,
        size_t chunkHeaderSize, size_t maxElementSize, size_t maxAlignment, size_t offset);

    ~GrowingPoolAllocator(void);

    X_INLINE void* allocate(size_t size, size_t alignment, size_t offset);

    // Allocates raw memory, and additionally fills newly allocated physical memory chunks with the given header data.
    // The returned pointer will always adhere to the following alignment requirements: <tt>((ptr + offset) % alignment) == 0</tt>.
    template<class T>
    X_INLINE void* allocate(size_t size, size_t alignment, size_t offset,
        typename std::enable_if<!core::compileTime::IsPointer<T>::Value, T>::type& chunkHeader);

    X_INLINE void free(void* ptr);
    X_INLINE void free(void* ptr, size_t size);

    X_INLINE size_t getSize(void* allocation) const;
    X_INLINE size_t usableSize(void* ptr) const;

    MemoryAllocatorStatistics getStatistics(void) const;

    // Returns whether a given allocation belongs to this allocator.
    // This is done by checking if the allocation resides in the virtual address space of the allocator.
    X_INLINE bool containsAllocation(void* ptr) const;

    // Returns the address of the chunk header for a given allocation.
    // The address of a chunk header is determined by rounding the address to the next multiple of the given
    // chunk size, and subtracting the size of the header.
    template<typename T>
    static X_INLINE T* getChunkHeader(void* allocationAddress, unsigned int chunkSize, unsigned int chunkHeaderSize);

private:
    // Allocates raw memory, and additionally fills newly allocated physical memory chunks with the given header data (if any).
    void* allocate(size_t size, size_t alignment, size_t offset, const void* chunkHeaderData, size_t chunkHeaderSize);

    Freelist freelist_;
    char* virtualStart_;
    char* virtualEnd_;
    char* physicalCurrent_;
    size_t growSize_;
    size_t chunkHeaderSize_;
    size_t maxSize_;
    size_t maxAlignment_;

#if X_ENABLE_POOL_ALLOCATOR_CHECK
    size_t offset_;
#endif

#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
    size_t elementSize_;
    size_t wastePerElement_;
    MemoryAllocatorStatistics statistics_;
#endif
};

#include "GrowingPoolAllocator.inl"

X_NAMESPACE_END

#endif
