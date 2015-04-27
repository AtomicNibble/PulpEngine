#include "EngineCommon.h"


#include "LinearAllocator.h"
#include "Util\PointerUtil.h"


X_NAMESPACE_BEGIN(core)



LinearAllocator::LinearAllocator(void* start, void* end) :
	m_start( (char*)start ),
	m_end( (char*)end ),
	m_current( (char*)start )
{
	
#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
	zero_this( &m_statistics );

	size_t Len = static_cast<size_t>((uintptr_t)end - (uintptr_t)start);

	m_statistics.m_type = "Linear";
	m_statistics.m_virtualMemoryReserved = Len;
	m_statistics.m_physicalMemoryAllocated = Len;
	m_statistics.m_physicalMemoryAllocatedMax = Len;
#endif
}


void* LinearAllocator::allocate(size_t size, size_t alignment, size_t align_offset)
{
#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
	// used to work out alignment waste.
	// const uint32_t allocationOffset = safe_static_cast<uint32_t>(m_current - m_start);
	char* oldCurrent = m_current;
#endif

	size += sizeof(size_t); // add room for our book keeping.
	align_offset += sizeof(size_t); // offset includes base position.

	m_current = pointerUtil::AlignTop(m_current + align_offset, alignment) - align_offset;

	if ((m_current + size) > m_end) { // check for overflow.
		X_ASSERT(false, "Stack overflow!, a linear allocator can't satisfy the request.")(size, m_end - m_start);
		return nullptr;
	}

#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
	// stats baby !
	m_statistics.m_allocationCount++;
	m_statistics.m_internalOverhead += sizeof(size_t);
	m_statistics.m_wasteAlignment += safe_static_cast<size_t>((uintptr_t)m_current - (uintptr_t)oldCurrent);

#endif

	union
	{
		void* as_void;
		char* as_char;
		size_t* as_size_t;
	};

	as_void = m_current;
	*as_size_t = (size - sizeof(size_t)); // store out size.
	as_char += sizeof(size_t);

	void* userPtr = as_char; // save the pointer we return.
	m_current += size; // set current position.

#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
	// stats baby !
	m_statistics.m_physicalMemoryUsed = safe_static_cast<size_t>(m_current - m_start);

	m_statistics.m_wasteAlignmentMax = Max( m_statistics.m_wasteAlignment, m_statistics.m_wasteAlignmentMax );
	m_statistics.m_allocationCountMax = Max( m_statistics.m_allocationCount, m_statistics.m_allocationCountMax );
	m_statistics.m_internalOverheadMax = Max( m_statistics.m_internalOverhead, m_statistics.m_internalOverheadMax );
	m_statistics.m_physicalMemoryUsedMax = Max( m_statistics.m_physicalMemoryUsed, m_statistics.m_physicalMemoryUsedMax );
#endif


	return userPtr;
}


MemoryAllocatorStatistics LinearAllocator::getStatistics(void) const
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