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

	m_statistics.type_ = "Pool";
	m_statistics.virtualMemoryReserved_ = Size;
	m_statistics.physicalMemoryAllocated_ = Size;
	m_statistics.physicalMemoryUsed_ = Waste;
	m_statistics.wasteAlignment_ = Waste;


	m_statistics.wasteUnused_ = (Size - Waste) - (m_elementSize * ((Size - Waste) / m_elementSize));

	m_statistics.wasteAlignmentMax_ = Max( m_statistics.wasteAlignment_, m_statistics.wasteAlignmentMax_ );
	m_statistics.wasteUnusedMax_ = Max( m_statistics.wasteUnused_, m_statistics.wasteUnusedMax_ );
	m_statistics.allocationCountMax_ = Max( m_statistics.allocationCount_, m_statistics.allocationCountMax_ );
	m_statistics.internalOverheadMax_ = Max( m_statistics.internalOverhead_, m_statistics.internalOverheadMax_ );
	m_statistics.physicalMemoryUsedMax_ = Max(m_statistics.physicalMemoryUsed_, m_statistics.physicalMemoryUsedMax_);
	m_statistics.physicalMemoryAllocatedMax_ = Max(m_statistics.physicalMemoryAllocated_, m_statistics.physicalMemoryAllocatedMax_);
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