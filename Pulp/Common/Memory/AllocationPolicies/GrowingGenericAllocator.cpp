#include "EngineCommon.h"

#include "GrowingGenericAllocator.h"


X_NAMESPACE_BEGIN(core)



GrowingGenericAllocator::GrowingGenericAllocator( uint32_t maxSizeInBytesPerPool, uint32_t growSize, size_t maxAlignment, size_t offset) :
	m_microAllocator( maxSizeInBytesPerPool, growSize, maxAlignment, offset ),
	m_blockAllocator()
{

#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
	zero_object( m_statistics );

	m_statistics.m_type = "GrowingGeneric";
#endif
}


/// Returns statistics regarding the allocations made by the allocator.
MemoryAllocatorStatistics GrowingGenericAllocator::getStatistics(void) const
{
#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
	return m_statistics;
#else
	static MemoryAllocatorStatistics stats;
	core::zero_object(stats);
	return stats;
#endif
}


#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
void GrowingGenericAllocator::updateStatistics(void)
{
	MemoryAllocatorStatistics Micro = m_microAllocator.getStatistics();
	MemoryAllocatorStatistics Block = m_blockAllocator.getStatistics();


	m_statistics.Clear();
	m_statistics += Micro;
	m_statistics += Block;
}
#endif




X_NAMESPACE_END