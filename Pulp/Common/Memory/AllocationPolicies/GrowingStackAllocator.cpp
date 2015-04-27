#include "EngineCommon.h"

#include "GrowingStackAllocator.h"

#include "Util/BitUtil.h"
#include "memory/VirtualMem.h"

X_NAMESPACE_BEGIN(core)

GrowingStackAllocator::GrowingStackAllocator(size_t maxSizeInBytes, size_t granularity)
{
	m_virtualStart = (char*)VirtualMem::ReserveAddressSpace(maxSizeInBytes);
	m_virtualEnd = &m_virtualStart[maxSizeInBytes];
	m_physicalCurrent = m_virtualStart;
	m_physicalEnd = m_virtualStart;
	m_granularity = granularity;
#if X_ENABLE_STACK_ALLOCATOR_CHECK
	m_allocationID = 0;
#endif

	if (!bitUtil::IsPowerOfTwo(granularity))
	{
		X_ASSERT("GrowingStackAlloc", "Granularity size must be a power of 2")(granularity);
	}
	if (maxSizeInBytes % granularity)
	{
		X_ASSERT("GrowingStackAlloc", "max size is not a multiple of growsize")(maxSizeInBytes, granularity);
	}
	if (granularity % VirtualMem::GetPageSize())
	{
		X_ASSERT("GrowingStackAlloc", "Granularity size must be a multiple of virtual page size")(granularity, VirtualMem::GetPageSize());
	}

#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
	core::zero_object(m_statistics);
	m_statistics.m_type = "GrowingStackAlloc";
	m_statistics.m_virtualMemoryReserved = maxSizeInBytes;
#endif 
}

GrowingStackAllocator::~GrowingStackAllocator(void)
{
	VirtualMem::ReleaseAddressSpace(m_virtualStart);
}


void* GrowingStackAllocator::allocate(size_t size, size_t alignment, size_t offset)
{
	size_t neededPhysicalSize;
	size_t allocationOffset;
	size_t allocationSize;
	char* oldCurrent;


	allocationSize = size; // Requested size
	allocationOffset = safe_static_cast<size_t>(m_physicalCurrent - m_virtualStart);
	oldCurrent = m_physicalCurrent;
	size += sizeof(BlockHeader); // add room for our book keeping.

	// Get aligned location.
	m_physicalCurrent = pointerUtil::AlignTop<char>(
					&m_physicalCurrent[offset + sizeof(BlockHeader)], alignment)
					- (offset + sizeof(BlockHeader));

	// space?
	if (&m_physicalCurrent[size] > m_physicalEnd)
	{
		neededPhysicalSize = bitUtil::RoundUpToMultiple(size, m_granularity);

		if (&m_physicalEnd[neededPhysicalSize] <= m_virtualEnd)
		{
			VirtualMem::AllocatePhysicalMemory(m_physicalEnd, neededPhysicalSize);
			m_physicalEnd += neededPhysicalSize;
		}
	}

#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
		++m_statistics.m_allocationCount;
		m_statistics.m_allocationCountMax = core::Max<size_t>(m_statistics.m_allocationCount, m_statistics.m_allocationCountMax);
		m_statistics.m_physicalMemoryAllocated = safe_static_cast<size_t>(m_physicalEnd - m_virtualStart);
		m_statistics.m_physicalMemoryAllocatedMax = core::Max<size_t>(m_statistics.m_physicalMemoryAllocated, m_statistics.m_physicalMemoryAllocatedMax);
		m_statistics.m_physicalMemoryUsed = safe_static_cast<size_t>(m_physicalCurrent - m_virtualStart);
		m_statistics.m_physicalMemoryUsedMax = core::Max<size_t>(m_statistics.m_physicalMemoryUsed, m_statistics.m_physicalMemoryUsedMax);
		m_statistics.m_wasteAlignment += safe_static_cast<size_t>((uintptr_t)(m_physicalCurrent + (uintptr_t)oldCurrent - size));
		m_statistics.m_wasteAlignmentMax = core::Max<size_t>(m_statistics.m_wasteAlignment, m_statistics.m_wasteAlignmentMax);
		m_statistics.m_internalOverhead += sizeof(BlockHeader);
		m_statistics.m_internalOverheadMax = core::Max<size_t>(m_statistics.m_internalOverhead, m_statistics.m_internalOverheadMax);
#endif
	

	union
	{
		void* as_void;
		char* as_char;
		BlockHeader* as_header;
	};

	as_char = m_physicalCurrent;
#if X_ENABLE_STACK_ALLOCATOR_CHECK
	as_header->m_AllocationID = m_allocationID++;
#endif
	as_header->m_allocationOffset = allocationOffset;
	as_header->m_AllocationSize = allocationSize;
	as_char += sizeof(BlockHeader);

	void* userPtr = as_void;
	m_physicalCurrent += size;
	return userPtr;
}

void GrowingStackAllocator::free(void* ptr)
{
	X_ASSERT_NOT_NULL(ptr);

	union
	{
		void* as_void;
		char* as_char;
		BlockHeader* as_header;
	};

	as_void = ptr;
	as_char -= sizeof(BlockHeader);

#if X_ENABLE_STACK_ALLOCATOR_CHECK
	if (as_header->m_AllocationID != (m_allocationID - 1))
	{
		uint32_t AllocationID = as_header->m_AllocationID;
		X_ASSERT(false, "Cannot free memory from stack(LIFO). invalid order.")(m_allocationID, AllocationID, ptr);
	}

	m_allocationID--;
#endif

	m_physicalCurrent = &m_virtualStart[as_header->m_allocationOffset];


#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
	m_statistics.m_allocationCount--;
	m_statistics.m_physicalMemoryUsed = as_header->m_allocationOffset;
	m_statistics.m_internalOverhead -= sizeof(BlockHeader);
	m_statistics.m_wasteAlignment -= safe_static_cast<size_t>(as_char - m_physicalCurrent);
#endif
}

void GrowingStackAllocator::purge(void)
{
	char* start = pointerUtil::AlignTop<char>(m_physicalCurrent, m_granularity);
	size_t size = safe_static_cast<size_t>(m_physicalEnd - start);

	VirtualMem::FreePhysicalMemory(start,size);

	m_physicalEnd = start;

#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
	m_statistics.m_physicalMemoryAllocated = safe_static_cast<size_t>(m_physicalEnd - m_virtualStart);
#endif
}


MemoryAllocatorStatistics GrowingStackAllocator::getStatistics(void) const
{
#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
	return m_statistics;
#else
	MemoryAllocatorStatistics stats;
	core::zero_object(stats);
	return stats;
#endif
}

X_NAMESPACE_END