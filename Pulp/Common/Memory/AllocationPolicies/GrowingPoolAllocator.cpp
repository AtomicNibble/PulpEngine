#include "EngineCommon.h"

#include "GrowingPoolAllocator.h"

#include "Util/BitUtil.h"
#include "memory/VirtualMem.h"

X_NAMESPACE_BEGIN(core)


namespace {

	size_t CalculateElementSize( size_t maxElementSize, size_t maxAlignment )
	{
		return bitUtil::RoundUpToMultiple( maxElementSize, maxAlignment );
	}

	size_t CalculateWasteAtFront(void *start, size_t maxAlignment, size_t offset)
	{
		char* pAdd = pointerUtil::AlignTop( (char*)start + offset, maxAlignment );

		return safe_static_cast<uint32_t>( ( pAdd - offset ) - (char*)start );
	}

}


GrowingPoolAllocator::GrowingPoolAllocator(unsigned int maxSizeInBytes, unsigned int growSize, 
	unsigned int chunkHeaderSize, size_t maxElementSize, size_t maxAlignment, size_t offset)
{
	m_virtualStart = static_cast<char*>( VirtualMem::ReserveAddressSpace( maxSizeInBytes ) );
	m_virtualEnd = &m_virtualStart[ maxSizeInBytes ];

	m_physicalCurrent = m_virtualStart;
	m_growSize = growSize;
	m_chunkHeaderSize = chunkHeaderSize;
	m_maxSize = maxElementSize;
	m_maxAlignment = maxAlignment;

#if X_ENABLE_POOL_ALLOCATOR_CHECK
	m_offset = offset;
#else
	X_UNUSED(offset);
#endif

	X_ASSERT( bitUtil::IsPowerOfTwo( growSize ), "Pool Grow size must be a power-of-two." )( growSize );

	if( maxSizeInBytes % growSize )
		X_ASSERT( false, "Maximum amount of virtual address space to reserve must be a multiple of the grow size." )( growSize );
	if( growSize % VirtualMem::GetPageSize() )
		X_ASSERT( false, "Pool grow size must be a multiple of virtual page size." )( growSize );
	

#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
	zero_object( m_statistics );

	m_statistics.m_virtualMemoryReserved = maxSizeInBytes;

	m_elementSize = CalculateElementSize( maxElementSize, maxAlignment );
	m_wastePerElement = m_elementSize = maxElementSize;

	m_statistics.m_type = "GrowPoolAllocator";
#endif

}

/// Frees all physical memory owned by the allocator.
GrowingPoolAllocator::~GrowingPoolAllocator(void)
{
	VirtualMem::ReleaseAddressSpace( m_virtualStart );
}



void* GrowingPoolAllocator::allocate( size_t size, size_t alignment, size_t offset, const void* chunkHeaderData, size_t chunkHeaderSize)
{
	if ( size > m_maxSize )
	{
		X_ASSERT( false, "Pool allocator can't satsify a request bigger than max size only equal or less" )( size, m_maxSize );
	}

	if ( alignment > m_maxAlignment )
	{
		X_ASSERT( false, "Pool allocaotr alignment must be equal or less" )( alignment, m_maxAlignment );
	}

#if X_ENABLE_POOL_ALLOCATOR_CHECK
	if ( offset != m_offset )
	{
		X_ASSERT( false, "A pool allocator can only allocate instances with the same offset." )( offset, m_offset );
	}
#endif

	void* pMem = m_freelist.Obtain();

	if( !pMem ) // we need to grow baby.
	{
		size_t neededPhysicalSize = bitUtil::RoundUpToMultiple<size_t>( offset + m_maxSize, m_growSize );

		if ( &m_physicalCurrent[ neededPhysicalSize ] <= m_virtualEnd )
		{
			void* pMemoryRegionStart = VirtualMem::AllocatePhysicalMemory( m_physicalCurrent, neededPhysicalSize );	
			m_physicalCurrent += neededPhysicalSize;
			void* pChunkHeaderStart = m_physicalCurrent - m_chunkHeaderSize;

			m_freelist = Freelist( pMemoryRegionStart, pChunkHeaderStart, m_maxSize, m_maxAlignment, offset );

			// ok now try.
			pMem = m_freelist.Obtain(); 

			if ( chunkHeaderData ) {
				memcpy( (char*)pChunkHeaderStart, (char*)chunkHeaderData, chunkHeaderSize);
			}

			// update all dat info.
#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
			size_t wasteAtFront = CalculateWasteAtFront(pMemoryRegionStart, m_maxAlignment, offset);
			size_t memoryRegionSize = safe_static_cast<uint32_t>( (char*)pChunkHeaderStart - (char*)pMemoryRegionStart );
			size_t elementCount = (memoryRegionSize - wasteAtFront) / m_elementSize;

			m_statistics.m_physicalMemoryAllocated = safe_static_cast<uint32_t>( m_physicalCurrent - m_virtualStart );
			m_statistics.m_physicalMemoryUsed += m_chunkHeaderSize + wasteAtFront;
			m_statistics.m_wasteAlignment += wasteAtFront;
			m_statistics.m_wasteUnused = m_chunkHeaderSize + memoryRegionSize - wasteAtFront- m_elementSize * elementCount;

			// 1,2,3 what is the max? I hope i don't have to pay tax.
			m_statistics.m_physicalMemoryAllocatedMax = Max( m_statistics.m_physicalMemoryAllocatedMax, m_statistics.m_physicalMemoryAllocated );
			m_statistics.m_physicalMemoryUsedMax = Max( m_statistics.m_physicalMemoryUsedMax, m_statistics.m_physicalMemoryUsed );
			m_statistics.m_wasteAlignmentMax = Max( m_statistics.m_wasteAlignmentMax, m_statistics.m_wasteAlignment );
			m_statistics.m_wasteUnusedMax = Max( m_statistics.m_wasteUnusedMax, m_statistics.m_wasteUnused );
#endif
		}
		else
		{
			return nullptr;
		}
	}

	// info for when we either grow or not.
#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS

	m_statistics.m_allocationCount++;
	m_statistics.m_physicalMemoryUsed += m_elementSize;
	m_statistics.m_wasteAlignment += m_wastePerElement;

	m_statistics.m_allocationCountMax = Max( m_statistics.m_allocationCountMax, m_statistics.m_allocationCount );	
	m_statistics.m_physicalMemoryUsedMax = Max( m_statistics.m_physicalMemoryUsedMax, m_statistics.m_physicalMemoryUsed );
	m_statistics.m_wasteAlignmentMax = Max( m_statistics.m_wasteAlignmentMax, m_statistics.m_wasteAlignment );
#endif
	return pMem;
}


MemoryAllocatorStatistics GrowingPoolAllocator::getStatistics(void) const
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