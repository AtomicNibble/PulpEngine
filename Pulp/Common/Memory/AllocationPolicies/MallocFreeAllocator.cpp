#include "EngineCommon.h"

#include "MallocFreeAllocator.h"

#include "Util\PointerUtil.h"


X_NAMESPACE_BEGIN(core)


namespace
{
//	static const size_t SIZE_OF_HEADER = sizeof(MallocFreeAllocator::BlockHeader);

#ifdef _WIN64
	static_assert(MallocFreeAllocator::SIZE_OF_HEADER == 24, "Allocation header has wrong size.");
#else
	static_assert(MallocFreeAllocator::SIZE_OF_HEADER == 12, "Allocation header has wrong size.");
#endif
}



/// Default constructor.
MallocFreeAllocator::MallocFreeAllocator(void)
{
#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
	zero_object( m_statistics );

	m_statistics.type_ = "MallocFree";
#endif
}

/// \brief Allocates raw memory.
/// \remark The returned pointer will always adhere to the following alignment requirements: <tt>((ptr + offset) % alignment) == 0</tt>.
void* MallocFreeAllocator::allocate(size_t Origsize, size_t alignment, size_t offset)
{
	size_t size = Origsize + alignment + (SIZE_OF_HEADER);

	void* pMem = malloc( size );

	if( pMem )
	{
		union
		{
			BlockHeader* as_header;
			void* as_void;
			char* as_byte;
		};

		// get the aligned pointer.
		as_void = pointerUtil::AlignBottom<char>( (char*)pMem + offset + alignment + (SIZE_OF_HEADER-1), alignment );
		as_byte -= offset; // take off any offset.

		// we have taken off offfset so now header is just at -1 index.
		as_header[-1].m_originalAllocation = pMem;
		as_header[-1].m_AllocationSize = size;
		as_header[-1].m_originalSize = Origsize;


#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
		m_statistics.allocationCount_++;
		m_statistics.virtualMemoryReserved_ += size;
		m_statistics.physicalMemoryAllocated_ += size;
		m_statistics.physicalMemoryUsed_ += size;
		m_statistics.internalOverhead_ += SIZE_OF_HEADER;
		m_statistics.wasteAlignment_ += safe_static_cast<size_t>((uintptr_t)as_byte - (uintptr_t)pMem) - SIZE_OF_HEADER;


		m_statistics.allocationCountMax_ = Max( m_statistics.allocationCount_, m_statistics.allocationCountMax_ );
		m_statistics.physicalMemoryUsedMax_ = Max( m_statistics.physicalMemoryUsed_, m_statistics.physicalMemoryUsedMax_ );
		m_statistics.physicalMemoryAllocatedMax_ = Max( m_statistics.physicalMemoryAllocated_, m_statistics.physicalMemoryAllocatedMax_ );
		m_statistics.wasteAlignmentMax_ = Max( m_statistics.wasteAlignment_, m_statistics.wasteAlignmentMax_ );
		m_statistics.internalOverheadMax_ = Max( m_statistics.internalOverhead_, m_statistics.internalOverheadMax_ );
#endif
		
		return as_byte; // we return the mem - offset.
	}

	return nullptr;
}

/// Frees an allocation
void MallocFreeAllocator::free(void* ptr)
{
	X_ASSERT_NOT_NULL(ptr);

	union
	{
		BlockHeader* as_header;
		void* as_void;
		char* as_byte;
	};

	as_void = ptr;
	as_byte -= SIZE_OF_HEADER;

	
#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
	size_t AlignmentWaste = safe_static_cast<size_t>((uintptr_t)ptr - (uintptr_t)as_header->m_originalAllocation) - SIZE_OF_HEADER;
	size_t Size = as_header->m_AllocationSize;

	m_statistics.allocationCount_--;
	m_statistics.virtualMemoryReserved_ -= Size;
	m_statistics.physicalMemoryAllocated_ -= Size;
	m_statistics.physicalMemoryUsed_ -= Size;
	m_statistics.internalOverhead_ -= SIZE_OF_HEADER;
	m_statistics.wasteAlignment_ -= AlignmentWaste;
#endif

	::free( as_header->m_originalAllocation );
}



/// Returns statistics regarding the allocations made by the allocator.
MemoryAllocatorStatistics MallocFreeAllocator::getStatistics(void) const
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