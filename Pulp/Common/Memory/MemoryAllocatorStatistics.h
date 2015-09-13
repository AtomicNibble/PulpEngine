#pragma once

#ifndef X_MEMORYALLOCATORSTATISTICS_H_
#define X_MEMORYALLOCATORSTATISTICS_H_


X_NAMESPACE_BEGIN(core)



struct MemoryAllocatorStatistics
{
	// general
	const char* type_;									///< A human-readable string describing the type of allocator.

	// allocations
	size_t allocationCount_;							///< The current number of allocations.
	size_t allocationCountMax_;						///< The maximum number of allocations in flight at any given point in time.

	// virtual memory
	size_t virtualMemoryReserved_;						///< The amount of virtual memory reserved.

	// physical memory
	size_t physicalMemoryAllocated_;					///< The amount of physical memory currently allocated.
	size_t physicalMemoryAllocatedMax_;				///< The maximum amount of physical memory that was allocated.
	size_t physicalMemoryUsed_;						///< The amount of physical memory actually in use.
	size_t physicalMemoryUsedMax_;						///< The maximum amount of physical memory that was in use.

	// overhead
	size_t wasteAlignment_;							///< The amount of wasted memory due to alignment.
	size_t wasteAlignmentMax_;							///< The maximum amount of memory that was wasted due to alignment.
	size_t wasteUnused_;								///< The amount of memory that cannot be used for satisfying allocation requests.
	size_t wasteUnusedMax_;							///< The maximum amount of memory that could not be used for satisfying allocation requests.
	size_t internalOverhead_;							///< The internal overhead caused by book-keeping information.
	size_t internalOverheadMax_;						///< The maximum amount of internal overhead that was caused by book-keeping information.

	void Clear() {
		// don't clear the char pointer.
		memset( &allocationCount_, 0, sizeof( MemoryAllocatorStatistics ) - sizeof( type_ ) );
	}

	MemoryAllocatorStatistics& operator +=(const MemoryAllocatorStatistics& oth) {

		allocationCount_ += oth.allocationCount_;
		allocationCountMax_ += oth.allocationCountMax_;

		virtualMemoryReserved_ += oth.virtualMemoryReserved_;

		physicalMemoryAllocated_ += oth.physicalMemoryAllocated_;
		physicalMemoryAllocatedMax_ += oth.physicalMemoryAllocatedMax_;
		physicalMemoryUsed_ += oth.physicalMemoryUsed_;
		physicalMemoryUsedMax_ += oth.physicalMemoryUsedMax_;

		wasteAlignment_ += oth.wasteAlignment_;
		wasteAlignmentMax_ += oth.wasteAlignmentMax_;
		wasteUnused_ += oth.wasteUnused_;
		wasteUnusedMax_ += oth.wasteUnusedMax_;
		internalOverhead_ += oth.internalOverhead_;
		internalOverheadMax_ += oth.internalOverheadMax_;


		return *this;
	}
};

X_NAMESPACE_END


#endif // X_MEMORYALLOCATORSTATISTICS_H_
