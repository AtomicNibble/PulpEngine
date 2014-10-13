#pragma once

#ifndef X_MEMORYALLOCATORSTATISTICS_H_
#define X_MEMORYALLOCATORSTATISTICS_H_


X_NAMESPACE_BEGIN(core)



struct MemoryAllocatorStatistics
{
	// general
	const char* m_type;									///< A human-readable string describing the type of allocator.

	// allocations
	size_t m_allocationCount;							///< The current number of allocations.
	size_t m_allocationCountMax;						///< The maximum number of allocations in flight at any given point in time.

	// virtual memory
	size_t m_virtualMemoryReserved;						///< The amount of virtual memory reserved.

	// physical memory
	size_t m_physicalMemoryAllocated;					///< The amount of physical memory currently allocated.
	size_t m_physicalMemoryAllocatedMax;				///< The maximum amount of physical memory that was allocated.
	size_t m_physicalMemoryUsed;						///< The amount of physical memory actually in use.
	size_t m_physicalMemoryUsedMax;						///< The maximum amount of physical memory that was in use.

	// overhead
	size_t m_wasteAlignment;							///< The amount of wasted memory due to alignment.
	size_t m_wasteAlignmentMax;							///< The maximum amount of memory that was wasted due to alignment.
	size_t m_wasteUnused;								///< The amount of memory that cannot be used for satisfying allocation requests.
	size_t m_wasteUnusedMax;							///< The maximum amount of memory that could not be used for satisfying allocation requests.
	size_t m_internalOverhead;							///< The internal overhead caused by book-keeping information.
	size_t m_internalOverheadMax;						///< The maximum amount of internal overhead that was caused by book-keeping information.

	void Clear() {
		// don't clear the char pointer.
		memset( &m_allocationCount, 0, sizeof( MemoryAllocatorStatistics ) - sizeof( m_type ) );
	}

	MemoryAllocatorStatistics& operator +=(MemoryAllocatorStatistics& oth) {

		m_allocationCount += oth.m_allocationCount;
		m_allocationCountMax += oth.m_allocationCountMax;

		m_virtualMemoryReserved += oth.m_virtualMemoryReserved;

		m_physicalMemoryAllocated += oth.m_physicalMemoryAllocated;
		m_physicalMemoryAllocatedMax += oth.m_physicalMemoryAllocatedMax;
		m_physicalMemoryUsed += oth.m_physicalMemoryUsed;
		m_physicalMemoryUsedMax += oth.m_physicalMemoryUsedMax;

		m_wasteAlignment += oth.m_wasteAlignment;
		m_wasteAlignmentMax += oth.m_wasteAlignmentMax;
		m_wasteUnused += oth.m_wasteUnused;
		m_wasteUnusedMax += oth.m_wasteUnusedMax;
		m_internalOverhead += oth.m_internalOverhead;
		m_internalOverheadMax += oth.m_internalOverheadMax;


		return *this;
	}
};

X_NAMESPACE_END


#endif // X_MEMORYALLOCATORSTATISTICS_H_
