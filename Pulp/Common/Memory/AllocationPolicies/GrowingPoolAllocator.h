#pragma once

#ifndef X_GROWINGPOOLALLOCATOR_H
#define X_GROWINGPOOLALLOCATOR_H

#include "Containers/Freelist.h"
#include "Util/PointerUtil.h"
#include "Memory/MemoryAllocatorStatistics.h"


X_NAMESPACE_BEGIN(core)

/// \ingroup Memory
/// \brief A class that implements an allocation policy for memory arenas.
/// \details This class implements the concepts of an allocation policy as expected by the MemoryArena class.
///
/// The allocation scheme employed by this class is that of a growing pool allocator. The allocator behaves similar
/// to the non-growing PoolAllocator, but is not restricted to working within a fixed region of memory. This allocator
/// will initially only reserve virtual address space without allocating any physical memory. As the allocator needs to
/// grow, more and more physical memory is allocated, up to a given maximum size. Whenever an allocations is freed, the
/// allocator doesn't return physical memory to the OS.
///
/// Growing is implemented by making use of the virtual memory system. At first, the given amount of virtual address
/// space is reserved without allocating any physical memory. Whenever an allocation no longer fits into the currently
/// allocated physical memory, additional memory will be requested from the OS. The granularity with which the allocator
/// grows is provided by the user.
///
/// This allocator also supports custom per-chunk headers which can be used for storing user-data for each chunk.
/// Whenever the allocator grows and requests a new chunk of memory from the OS, the last N bytes are reserved
/// for filling it with header data.
///
/// \remark Statistics gathering can be enabled/disabled via the preprocessor option \ref X_ENABLE_MEMORY_ALLOCATOR_STATISTICS.
/// If statistics are disabled, the allocator will still return a valid MemoryAllocatorStatistics object which is
/// initialized to its default values.
/// \remark Checking for wrong usage of the pool allocator can be enabled/disabled via the preprocessor option
/// \ref X_ENABLE_POOL_ALLOCATOR_CHECK. Enabling the check causes a small performance hit for
/// each allocation, and should therefore be disabled in retail builds.
class GrowingPoolAllocator
{
public:
	/// \brief Constructs a pool allocator.
	/// \details The allocator will never grow larger than \a maxSizeInBytes, and only grows in \a growSize chunks. Each
	/// chunk will reserve \a chunkHeaderSize bytes at the end of a chunk in order to store user-supplied data.
	GrowingPoolAllocator(size_t maxSizeInBytes, size_t growSize,
		size_t chunkHeaderSize, size_t maxElementSize, size_t maxAlignment, size_t offset);

	/// Frees all physical memory owned by the allocator.
	~GrowingPoolAllocator(void);

	/// \brief Allocates raw memory.
	/// \remark The returned pointer will always adhere to the following alignment requirements: <tt>((ptr + offset) % alignment) == 0</tt>.
	X_INLINE void* allocate(size_t size, size_t alignment, size_t offset);

	/// \brief Allocates raw memory, and additionally fills newly allocated physical memory chunks with the given header data.
	/// \remark The returned pointer will always adhere to the following alignment requirements: <tt>((ptr + offset) % alignment) == 0</tt>.
	template <class T>
	X_INLINE void* allocate(size_t size, size_t alignment, size_t offset, 
		typename std::enable_if<!core::compileTime::IsPointer<T>::Value, T>::type& chunkHeader);

	/// Frees an allocation.
	X_INLINE void free(void* ptr);
	X_INLINE void free(void* ptr, size_t size);

	/// Returns the original allocation size for an allocation.
	X_INLINE size_t getSize(void* allocation) const;

	X_INLINE size_t usableSize(void* ptr) const;

	/// Returns statistics regarding the allocations made by the allocator.
	MemoryAllocatorStatistics getStatistics(void) const;

	/// \brief Returns whether a given allocation belongs to this allocator.
	/// \details This is done by checking if the allocation resides in the virtual address space of the allocator.
	X_INLINE bool containsAllocation(void* ptr) const;

	/// \brief Returns the address of the chunk header for a given allocation.
	/// \details The address of a chunk header is determined by rounding the address to the next multiple of the given
	/// chunk size, and subtracting the size of the header.
	template <typename T>
	static X_INLINE T* getChunkHeader(void* allocationAddress, unsigned int chunkSize, unsigned int chunkHeaderSize);

private:
	/// Allocates raw memory, and additionally fills newly allocated physical memory chunks with the given header data (if any).
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
