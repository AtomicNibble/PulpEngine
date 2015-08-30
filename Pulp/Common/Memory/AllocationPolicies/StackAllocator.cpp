#include "EngineCommon.h"

#include "StackAllocator.h"

#include "Util\PointerUtil.h"

X_NAMESPACE_BEGIN(core)



StackAllocator::StackAllocator(void* start, void* end) :
	m_start( (char*)start ),
	m_end( (char*)end ),
	m_current( (char*)start )
{
#if X_ENABLE_STACK_ALLOCATOR_CHECK
	m_allocationID = 0;
#endif
	
#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
	zero_this( &m_statistics );

	size_t Len = safe_static_cast<size_t, uintptr_t>((uintptr_t)end - (uintptr_t)start);

	m_statistics.m_type = "Stack";
	m_statistics.m_virtualMemoryReserved = Len;
	m_statistics.m_physicalMemoryAllocated = Len;
	m_statistics.m_physicalMemoryAllocatedMax = Len;
#endif
}


void* StackAllocator::allocate( size_t size, size_t alignment, size_t align_offset )
{
	size_t AllocationSize = size;

	size += sizeof( BlockHeader ); // add room for our book keeping.
	align_offset += sizeof( BlockHeader ); // offset includes base position.

	// get current relative offset
	const uint32_t allocationOffset = safe_static_cast<uint32_t>(m_current - m_start);
#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
	const char* old_current = m_current;
#endif // !X_ENABLE_MEMORY_ALLOCATOR_STATISTICS

	// align it at the offset position, then go back the offset to bring us to start.
	// of return data not actual start as alignment can waste some memory.
	m_current = pointerUtil::AlignTop(m_current + align_offset, alignment) - align_offset;

	// Do we even have room slut?
	if( (m_current + size) > m_end ) {
		X_ASSERT((m_current + size) <= m_end, "Stack overflow!, a stack allocator can't satisfy the request.")();
		return nullptr;
	}
	

#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
	// stats baby !
	m_statistics.m_allocationCount++;
	m_statistics.m_internalOverhead += sizeof( BlockHeader );
	m_statistics.m_physicalMemoryUsed += size + (m_current - old_current);
	m_statistics.m_wasteAlignment += safe_static_cast<size_t>(((uintptr_t)m_current - (uintptr_t)m_start) - allocationOffset);

	m_statistics.m_wasteAlignmentMax = Max( m_statistics.m_wasteAlignment, m_statistics.m_wasteAlignmentMax );
	m_statistics.m_allocationCountMax = Max( m_statistics.m_allocationCount, m_statistics.m_allocationCountMax );
	m_statistics.m_internalOverheadMax = Max( m_statistics.m_internalOverhead, m_statistics.m_internalOverheadMax );
	m_statistics.m_physicalMemoryUsedMax = Max( m_statistics.m_physicalMemoryUsed, m_statistics.m_physicalMemoryUsedMax );

#endif

	union
	{
		void* as_void;
		char* as_char;
		BlockHeader* as_header;
	};

	as_char = m_current;
#if X_ENABLE_STACK_ALLOCATOR_CHECK
	as_header->m_AllocationID = m_allocationID++;
#endif
	as_header->m_allocationOffset = allocationOffset;
	as_header->m_AllocationSize = AllocationSize;
	as_char += sizeof( BlockHeader );

	void* userPtr = as_void;
	m_current += size;
	return userPtr;
}



void StackAllocator::free(void* ptr)
{
	X_ASSERT_NOT_NULL(ptr);

	union
	{
		void* as_void;
		char* as_char;
		BlockHeader* as_header;
	};

	as_void = ptr;
	as_char -= sizeof( BlockHeader );

#if X_ENABLE_STACK_ALLOCATOR_CHECK
	if( as_header->m_AllocationID != (m_allocationID-1) )
	{
		uint32_t AllocationID = as_header->m_AllocationID;
		X_ASSERT( false, "Cannot free memory from stack(LIFO). invalid order." )( m_allocationID, AllocationID );
	}

	m_allocationID--;
#endif

	m_current = m_start + as_header->m_allocationOffset;

#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
	m_statistics.m_allocationCount--;
	m_statistics.m_physicalMemoryUsed = as_header->m_allocationOffset;
	m_statistics.m_internalOverhead -= sizeof( BlockHeader );
	m_statistics.m_wasteAlignment -= safe_static_cast<uint32_t>( as_char - m_current );
#endif

}


MemoryAllocatorStatistics StackAllocator::getStatistics(void) const
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