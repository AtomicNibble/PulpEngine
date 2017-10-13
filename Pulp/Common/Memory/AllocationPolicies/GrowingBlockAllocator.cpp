#include "EngineCommon.h"

#include "GrowingBlockAllocator.h"
#include "Util/PointerUtil.h"


#if !X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
#define NO_MALLINFO
#define NO_MALLOC_STATS
#else
#define NO_MALLOC_STATS

#if X_ENABLE_MEMORY_ALLOCATOR_DMALLOC_STATISTICS
#define NO_MALLINFO
#endif // !X_ENABLE_MEMORY_ALLOCATOR_DMALLOC_STATISTICS

#endif // !X_ENABLE_MEMORY_ALLOCATOR_STATISTICS

#include "memory/dlmalloc-2.8.6.h"

X_NAMESPACE_BEGIN(core)


GrowingBlockAllocator::GrowingBlockAllocator(void)
{
	memorySpace_ = create_mspace(0,0);

	
#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
	zero_object( statistics_ );

	statistics_.type_ = "GrowBlock";
#endif
}

GrowingBlockAllocator::~GrowingBlockAllocator(void)
{
	destroy_mspace( memorySpace_ );
}

void* GrowingBlockAllocator::allocate( size_t originalSize, size_t alignment, size_t offset )
{
	static const size_t BlockHeaderMem = sizeof( BlockHeader ) - 1;

	size_t size = alignment + BlockHeaderMem + originalSize;

	void* pMem = mspace_malloc( memorySpace_, size);

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

		as_header[-1].originalAllocation_ = pMem;
		as_header[-1].originalSize_ = originalSize;

#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
		statistics_.allocationCount_++;
		statistics_.internalOverhead_ += sizeof( BlockHeader );
		statistics_.wasteAlignment_ += safe_static_cast<size_t>((uintptr_t)as_void - (uintptr_t)pMem);

#if X_ENABLE_MEMORY_ALLOCATOR_DMALLOC_STATISTICS
		mallinfo info = mspace_mallinfo( memorySpace_ );
		statistics_.physicalMemoryUsed_ = info.uordblks;
		statistics_.physicalMemoryAllocated_ = info.uordblks;
		statistics_.physicalMemoryUsedMax_ = info.usmblks;
		statistics_.physicalMemoryAllocatedMax_ = info.usmblks;
#endif // !X_ENABLE_MEMORY_ALLOCATOR_DMALLOC_STATISTICS

		statistics_.allocationCountMax_ = Max( statistics_.allocationCount_, statistics_.allocationCountMax_ );
		statistics_.wasteAlignmentMax_ = Max( statistics_.wasteAlignment_, statistics_.wasteAlignmentMax_ );
		statistics_.internalOverheadMax_ = Max( statistics_.internalOverhead_, statistics_.internalOverheadMax_ );
#endif // !X_ENABLE_MEMORY_ALLOCATOR_STATISTICS

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
		BlockHeader* as_header;
	};

	as_void = ptr;

	--as_header;
	
#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
	statistics_.allocationCount_--;
	statistics_.internalOverhead_ -= sizeof( BlockHeader );
	statistics_.wasteAlignment_ -= safe_static_cast<size_t>(reinterpret_cast<uintptr_t>(ptr) - 
		reinterpret_cast<uintptr_t>(as_header->originalAllocation_));
#endif // !X_ENABLE_MEMORY_ALLOCATOR_STATISTICS

	mspace_free( memorySpace_, as_header->originalAllocation_ );

#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS && X_ENABLE_MEMORY_ALLOCATOR_DMALLOC_STATISTICS
	mallinfo info = mspace_mallinfo( memorySpace_ );
	statistics_.physicalMemoryUsed_ = info.uordblks;
	statistics_.physicalMemoryAllocated_ = info.uordblks;
#endif

}

void GrowingBlockAllocator::free(void* ptr, size_t size)
{
	X_UNUSED(size);
	free(ptr);
}



MemoryAllocatorStatistics GrowingBlockAllocator::getStatistics(void) const
{
#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
	return statistics_;
#else
	static MemoryAllocatorStatistics stats;
	core::zero_object(stats);
	return stats;
#endif
}



X_NAMESPACE_END