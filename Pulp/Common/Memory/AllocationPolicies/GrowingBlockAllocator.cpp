#include "EngineCommon.h"

#include "GrowingBlockAllocator.h"
#include "Util/PointerUtil.h"

#include "memory/dlmalloc-2.8.6.h"

X_NAMESPACE_BEGIN(core)


GrowingBlockAllocator::GrowingBlockAllocator(void)
{
	m_memorySpace = create_mspace(0,0);

	
#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
	zero_object( m_statistics );

	m_statistics.m_type = "GrowBlock";
#endif
}

GrowingBlockAllocator::~GrowingBlockAllocator(void)
{
	destroy_mspace( m_memorySpace );
}

void* GrowingBlockAllocator::allocate( size_t originalSize, size_t alignment, size_t offset )
{
	static const size_t BlockHeaderMem = sizeof( BlockHeader ) - 1;

	size_t size = alignment + BlockHeaderMem + originalSize;

	void* pMem = mspace_malloc( m_memorySpace, size);

	union
	{
		void* as_void;
		char* as_char;
		BlockHeader* as_header;
	};

	as_void = pMem;

	if( pMem )
	{
		as_void = pointerUtil::AlignBottom<char>( as_char + offset + alignment + BlockHeaderMem, alignment) - offset;

		as_header[-1].m_originalAllocation = pMem;
		as_header[-1].m_originalSize = originalSize;

#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
		m_statistics.m_allocationCount++;
		m_statistics.m_internalOverhead += sizeof( BlockHeader );
		m_statistics.m_wasteAlignment += safe_static_cast<size_t>((uintptr_t)as_void - (uintptr_t)pMem);

		mallinfo info = mspace_mallinfo( m_memorySpace );

		m_statistics.m_physicalMemoryUsed = info.uordblks;
		m_statistics.m_physicalMemoryAllocated = info.uordblks;
		m_statistics.m_physicalMemoryUsedMax = info.usmblks;
		m_statistics.m_physicalMemoryAllocatedMax = info.usmblks;
		

		m_statistics.m_allocationCountMax = Max( m_statistics.m_allocationCount, m_statistics.m_allocationCountMax );
		m_statistics.m_wasteAlignmentMax = Max( m_statistics.m_wasteAlignment, m_statistics.m_wasteAlignmentMax );
		m_statistics.m_internalOverheadMax = Max( m_statistics.m_internalOverhead, m_statistics.m_internalOverheadMax );
#endif

		return as_void;
	}

	return pMem;
}

void GrowingBlockAllocator::free( void* ptr )
{
	X_ASSERT_NOT_NULL(ptr);

	union
	{
		void* as_void;
		char* as_char;
		BlockHeader* as_header;
	};

	as_void = ptr;

	--as_header;
	
#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
	m_statistics.m_allocationCount--;
	m_statistics.m_internalOverhead -= sizeof( BlockHeader );
	m_statistics.m_wasteAlignment -= safe_static_cast<size_t>((uintptr_t)ptr - (uintptr_t)as_header->m_originalAllocation);
#endif

	mspace_free( m_memorySpace, as_header->m_originalAllocation );

#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
	mallinfo info = mspace_mallinfo( m_memorySpace );

	m_statistics.m_physicalMemoryUsed = info.uordblks;
	m_statistics.m_physicalMemoryAllocated = info.uordblks;
#endif

}



MemoryAllocatorStatistics GrowingBlockAllocator::getStatistics(void) const
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