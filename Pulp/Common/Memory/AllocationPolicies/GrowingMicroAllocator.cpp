#include "EngineCommon.h"


#include "GrowingMicroAllocator.h"


X_NAMESPACE_BEGIN(core)


GrowingMicroAllocator::GrowingMicroAllocator( uint32_t maxSizeInBytesPerPool, uint32_t growSize, size_t maxAlignment, size_t offset ) :
	m_poolAllocator8( maxSizeInBytesPerPool, growSize, sizeof(ChunkHeader), 0x8, maxAlignment, offset ),
	m_poolAllocator16( maxSizeInBytesPerPool, growSize, sizeof(ChunkHeader), 0x10, maxAlignment, offset ),
	m_poolAllocator32( maxSizeInBytesPerPool, growSize, sizeof(ChunkHeader), 0x20, maxAlignment, offset ),
	m_poolAllocator64( maxSizeInBytesPerPool, growSize, sizeof(ChunkHeader), 0x40, maxAlignment, offset ),
	m_poolAllocator128( maxSizeInBytesPerPool, growSize, sizeof(ChunkHeader), 0x80, maxAlignment, offset ),
	m_poolAllocator256( maxSizeInBytesPerPool, growSize, sizeof(ChunkHeader), 0x100, maxAlignment, offset )
{
	m_chunkSize = growSize;


	// setup lookup table.
	uint i = 0;

	for( ; i<=8; i++ )
		m_poolAllocators[ i ] = &m_poolAllocator8;
	for( ; i<=0x10; i++ )
		m_poolAllocators[ i ] = &m_poolAllocator16;
	for( ; i<=0x20; i++ )
		m_poolAllocators[ i ] = &m_poolAllocator32;
	for( ; i<=0x40; i++ )
		m_poolAllocators[ i ] = &m_poolAllocator64;
	for( ; i<=0x80; i++ )
		m_poolAllocators[ i ] = &m_poolAllocator128;
	for( ; i<=0x100; i++ )
		m_poolAllocators[ i ] = &m_poolAllocator256;

#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS

	zero_object( m_statistics );

	m_statistics.m_type = "MicroPool";

#endif
}

void* GrowingMicroAllocator::allocate( size_t size, size_t alignment, size_t offset )
{
	if( size > MAX_ALLOCATION_SIZE )
	{
		X_ASSERT( false, "Micro Can't allocate more than %d bytes", MAX_ALLOCATION_SIZE )( size );
	}

	X_ALIGNED_SYMBOL(ChunkHeader,4) chunkHeader;
	chunkHeader.m_allocatorIndex = size; /// safe_static_cast<uint32_t,size_t>(size);

	void* pMem = m_poolAllocators[ size ]->allocate( size, alignment, offset, &chunkHeader );

#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
	updateStatistics();
#endif

	return pMem;
}

void GrowingMicroAllocator::free( void* ptr )
{
	ChunkHeader* header = GrowingPoolAllocator::getChunkHeader<ChunkHeader>(ptr, m_chunkSize, sizeof(ChunkHeader));

	X_ASSERT_NOT_NULL( header );

	if( header->m_allocatorIndex > 0x100 )
	{
		size_t allocatorIndex = header->m_allocatorIndex;
		X_ASSERT( false, "Invalid allocation index" )( allocatorIndex );
	}

	m_poolAllocators[ header->m_allocatorIndex ]->free( ptr );

#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
	updateStatistics();
#endif
}

#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS

void GrowingMicroAllocator::updateStatistics(void)
{
//	memset( &m_statistics.m_allocationCount, 0, sizeof( m_statistics ) - sizeof( m_statistics.m_type ) );

	m_statistics.Clear();

	MemoryAllocatorStatistics Stats[6];
	MemoryAllocatorStatistics* pStats = Stats;

	Stats[0] = m_poolAllocator8.getStatistics();
	Stats[1] = m_poolAllocator16.getStatistics();
	Stats[2] = m_poolAllocator32.getStatistics();
	Stats[3] = m_poolAllocator64.getStatistics();
	Stats[4] = m_poolAllocator128.getStatistics();
	Stats[5] = m_poolAllocator256.getStatistics();

	lopi( 6 )
	{
		m_statistics.m_allocationCount += pStats->m_allocationCount;
		m_statistics.m_virtualMemoryReserved += pStats->m_virtualMemoryReserved;
		m_statistics.m_physicalMemoryAllocated += pStats->m_physicalMemoryAllocated;
		m_statistics.m_physicalMemoryUsed += pStats->m_physicalMemoryUsed;
		m_statistics.m_wasteAlignment += pStats->m_wasteAlignment;
		m_statistics.m_wasteUnused += pStats->m_wasteUnused;
		m_statistics.m_internalOverhead += pStats->m_internalOverhead;


		m_statistics.m_allocationCountMax += pStats->m_allocationCountMax;
		m_statistics.m_physicalMemoryAllocatedMax += pStats->m_physicalMemoryAllocatedMax;
		m_statistics.m_physicalMemoryUsedMax += pStats->m_physicalMemoryUsedMax;
		m_statistics.m_wasteAlignmentMax += pStats->m_wasteAlignmentMax;
		m_statistics.m_wasteUnusedMax += pStats->m_wasteUnusedMax;
		m_statistics.m_internalOverheadMax += pStats->m_internalOverheadMax;

		++pStats;
	}

}

#endif

MemoryAllocatorStatistics GrowingMicroAllocator::getStatistics(void) const
{
#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
	return m_statistics;
#else
	static MemoryAllocatorStatistics stats;
	core::zero_object(stats);
	return stats;
#endif
}





X_NAMESPACE_END