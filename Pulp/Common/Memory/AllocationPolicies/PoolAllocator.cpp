#include "EngineCommon.h"


#include "PoolAllocator.h"
#include "Util/BitUtil.h"
#include "Util/PointerUtil.h"

X_NAMESPACE_BEGIN(core)


namespace {


	size_t CalculateElementSize( size_t maxElementSize, size_t maxAlignment )
	{
		return bitUtil::RoundUpToMultiple(maxElementSize, maxAlignment);
	}


	size_t CalculateWasteAtFront(void *start, size_t maxAlignment, size_t offset)
	{
		char* pAdd = pointerUtil::AlignTop( (char*)start + offset, maxAlignment );

		return safe_static_cast<size_t>((pAdd - offset) - (char*)start);
	}

}


PoolAllocator::PoolAllocator(void* start, void* end, size_t maxElementSize, size_t maxAlignment, size_t offset) :
	m_freelist( start, end, maxElementSize, maxAlignment, offset ),
	m_maxSize( maxElementSize )

#if X_ENABLE_POOL_ALLOCATOR_CHECK
	,m_maxAlignment( maxAlignment )
	,m_offset( offset )
#endif
{

#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
	size_t ElemSize = CalculateElementSize(maxElementSize, maxAlignment);
	size_t Waste = CalculateWasteAtFront( start, maxAlignment, offset );
	size_t Size = safe_static_cast<size_t>((char*)end - (char*)start);

	m_elementSize = ElemSize;
	m_wastePerElement = ElemSize - maxElementSize;

	zero_object( m_statistics );

	m_statistics.m_type = "Pool";
	m_statistics.m_virtualMemoryReserved = Size;
	m_statistics.m_physicalMemoryAllocated = Size;
	m_statistics.m_physicalMemoryUsed = Waste;
	m_statistics.m_wasteAlignment = Waste;


	m_statistics.m_wasteUnused = (Size - Waste) - (m_elementSize * ((Size - Waste) / m_elementSize));

	m_statistics.m_wasteAlignmentMax = Max( m_statistics.m_wasteAlignment, m_statistics.m_wasteAlignmentMax );
	m_statistics.m_wasteUnusedMax = Max( m_statistics.m_wasteUnused, m_statistics.m_wasteUnusedMax );
	m_statistics.m_allocationCountMax = Max( m_statistics.m_allocationCount, m_statistics.m_allocationCountMax );
	m_statistics.m_internalOverheadMax = Max( m_statistics.m_internalOverhead, m_statistics.m_internalOverheadMax );
	m_statistics.m_physicalMemoryUsedMax = Max(m_statistics.m_physicalMemoryUsed, m_statistics.m_physicalMemoryUsedMax);
	m_statistics.m_physicalMemoryAllocatedMax = Max(m_statistics.m_physicalMemoryAllocated, m_statistics.m_physicalMemoryAllocatedMax);
#endif

}


MemoryAllocatorStatistics PoolAllocator::getStatistics(void) const
{
#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
	return this->m_statistics;
#else
	static MemoryAllocatorStatistics stats;
	core::zero_object(stats);
	return stats;
#endif
}



X_NAMESPACE_END