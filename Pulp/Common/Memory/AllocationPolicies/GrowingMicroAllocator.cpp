#include "EngineCommon.h"


#include "GrowingMicroAllocator.h"


X_NAMESPACE_BEGIN(core)


GrowingMicroAllocator::GrowingMicroAllocator( uint32_t maxSizeInBytesPerPool, uint32_t growSize, size_t maxAlignment, size_t offset ) :
	poolAllocator8_( maxSizeInBytesPerPool, growSize, sizeof(ChunkHeader), 0x8, maxAlignment, offset ),
	poolAllocator16_( maxSizeInBytesPerPool, growSize, sizeof(ChunkHeader), 0x10, maxAlignment, offset ),
	poolAllocator32_( maxSizeInBytesPerPool, growSize, sizeof(ChunkHeader), 0x20, maxAlignment, offset ),
	poolAllocator64_( maxSizeInBytesPerPool, growSize, sizeof(ChunkHeader), 0x40, maxAlignment, offset ),
	poolAllocator128_( maxSizeInBytesPerPool, growSize, sizeof(ChunkHeader), 0x80, maxAlignment, offset ),
	poolAllocator256_( maxSizeInBytesPerPool, growSize, sizeof(ChunkHeader), 0x100, maxAlignment, offset )
{
	chunkSize_ = growSize;

#if X_USE_FULL_LOOKUP_TABLE

	// setup lookup table.
	int32_t i = 0;

	for (; i <= 8; i++) {
		poolAllocators_[i] = &poolAllocator8_;
	}
	for (; i <= 0x10; i++) {
		poolAllocators_[i] = &poolAllocator16_;
	}
	for (; i <= 0x20; i++) {
		poolAllocators_[i] = &poolAllocator32_;
	}
	for (; i <= 0x40; i++) {
		poolAllocators_[i] = &poolAllocator64_;
	}
	for (; i <= 0x80; i++) {
		poolAllocators_[i] = &poolAllocator128_;
	}
	for (; i <= 0x100; i++) {
		poolAllocators_[i] = &poolAllocator256_;
	}

#else

	poolAllocators_[0] = &poolAllocator8_; 
	poolAllocators_[1] = &poolAllocator8_; 
	poolAllocators_[2] = &poolAllocator8_; 
	poolAllocators_[3] = &poolAllocator16_;  
	poolAllocators_[4] = &poolAllocator32_; 
	poolAllocators_[5] = &poolAllocator64_; 
	poolAllocators_[6] = &poolAllocator128_; 
	poolAllocators_[7] = &poolAllocator256_;

#endif // !X_USE_FULL_LOOKUP_TABLE

#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS

	zero_object( statistics_ );
	statistics_.type_ = "MicroPool";

#endif
}

void* GrowingMicroAllocator::allocate( size_t size, size_t alignment, size_t offset )
{
	if( size > MAX_ALLOCATION_SIZE )
	{
		X_ASSERT( false, "Micro Can't allocate more than %d bytes", MAX_ALLOCATION_SIZE )( size );
	}

#if X_USE_FULL_LOOKUP_TABLE
	static_assert(std::numeric_limits<decltype(ChunkHeader::allocatorIndex_)>::max() >= MAX_ALLOCATION_SIZE, "Can't store allocation indexes");
#else
	static_assert(std::numeric_limits<decltype(ChunkHeader::allocatorIndex_)>::max() >= X_ARRAY_SIZE(poolAllocators_), "Can't store allocation indexes");
#endif // !X_USE_FULL_LOOKUP_TABLE

	ChunkHeader chunkHeader;

#if X_USE_FULL_LOOKUP_TABLE
	chunkHeader.allocatorIndex_ = safe_static_cast<decltype(ChunkHeader::allocatorIndex_)>(size);

	void* pMem = poolAllocators_[size]->allocate<ChunkHeader>(size, alignment, offset, chunkHeader);
#else

	int32_t index = core::bitUtil::ScanBits<int32_t>(((int32_t)size) - 1);
	chunkHeader.allocatorIndex_ = safe_static_cast<decltype(ChunkHeader::allocatorIndex_)>(index);

	void* pMem = poolAllocators_[index]->allocate<ChunkHeader>(size, alignment, offset, chunkHeader);

#endif // !X_USE_FULL_LOOKUP_TABLE

#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
	updateStatistics();
#endif

	return pMem;
}

void GrowingMicroAllocator::free( void* ptr )
{
	ChunkHeader* header = GrowingPoolAllocator::getChunkHeader<ChunkHeader>(ptr, 
		chunkSize_, sizeof(ChunkHeader));

	X_ASSERT_NOT_NULL( header );

	poolAllocators_[header->allocatorIndex_]->free(ptr);

#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
	updateStatistics();
#endif
}

void GrowingMicroAllocator::free(void* ptr, size_t size)
{
	X_UNUSED(size);
	free(ptr);
}


#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS

void GrowingMicroAllocator::updateStatistics(void)
{
//	memset( &statistics_.allocationCount_, 0, sizeof( statistics_ ) - sizeof( statistics_.type_ ) );

	statistics_.Clear();

	MemoryAllocatorStatistics Stats[6];
	MemoryAllocatorStatistics* pStats = Stats;

	Stats[0] = poolAllocator8_.getStatistics();
	Stats[1] = poolAllocator16_.getStatistics();
	Stats[2] = poolAllocator32_.getStatistics();
	Stats[3] = poolAllocator64_.getStatistics();
	Stats[4] = poolAllocator128_.getStatistics();
	Stats[5] = poolAllocator256_.getStatistics();

	lopi( 6 )
	{
		statistics_.allocationCount_ += pStats->allocationCount_;
		statistics_.virtualMemoryReserved_ += pStats->virtualMemoryReserved_;
		statistics_.physicalMemoryAllocated_ += pStats->physicalMemoryAllocated_;
		statistics_.physicalMemoryUsed_ += pStats->physicalMemoryUsed_;
		statistics_.wasteAlignment_ += pStats->wasteAlignment_;
		statistics_.wasteUnused_ += pStats->wasteUnused_;
		statistics_.internalOverhead_ += pStats->internalOverhead_;


		statistics_.allocationCountMax_ += pStats->allocationCountMax_;
		statistics_.physicalMemoryAllocatedMax_ += pStats->physicalMemoryAllocatedMax_;
		statistics_.physicalMemoryUsedMax_ += pStats->physicalMemoryUsedMax_;
		statistics_.wasteAlignmentMax_ += pStats->wasteAlignmentMax_;
		statistics_.wasteUnusedMax_ += pStats->wasteUnusedMax_;
		statistics_.internalOverheadMax_ += pStats->internalOverheadMax_;

		++pStats;
	}

}

#endif

MemoryAllocatorStatistics GrowingMicroAllocator::getStatistics(void) const
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