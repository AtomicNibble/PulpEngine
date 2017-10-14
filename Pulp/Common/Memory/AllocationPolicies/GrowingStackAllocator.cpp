#include "EngineCommon.h"

#include "GrowingStackAllocator.h"

#include "Util/BitUtil.h"
#include "memory/VirtualMem.h"

X_NAMESPACE_BEGIN(core)

GrowingStackAllocator::GrowingStackAllocator(size_t maxSizeInBytes, size_t granularity)
{
	virtualStart_ = reinterpret_cast<char*>(VirtualMem::ReserveAddressSpace(maxSizeInBytes));
	virtualEnd_ = &virtualStart_[maxSizeInBytes];
	physicalCurrent_ = virtualStart_;
	physicalEnd_ = virtualStart_;
	granularity_ = granularity;
#if X_ENABLE_STACK_ALLOCATOR_CHECK
	allocationID_ = 0;
#endif

	if (!bitUtil::IsPowerOfTwo(granularity))
	{
		X_ASSERT("GrowingStackAlloc", "Granularity size must be a power of 2")(granularity);
	}
	if (maxSizeInBytes % granularity)
	{
		X_ASSERT("GrowingStackAlloc", "max size is not a multiple of growsize")(maxSizeInBytes, granularity);
	}
	if (granularity % VirtualMem::GetPageSize())
	{
		X_ASSERT("GrowingStackAlloc", "Granularity size must be a multiple of virtual page size")(granularity, VirtualMem::GetPageSize());
	}

#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
	core::zero_object(statistics_);
	statistics_.type_ = "GrowingStackAlloc";
	statistics_.virtualMemoryReserved_ = maxSizeInBytes;
#endif 
}

GrowingStackAllocator::~GrowingStackAllocator(void)
{
	VirtualMem::ReleaseAddressSpace(virtualStart_);
}


void* GrowingStackAllocator::allocate(size_t size, size_t alignment, size_t offset)
{
	size_t neededPhysicalSize;
	size_t allocationOffset;
	size_t allocationSize;
	char* oldCurrent;


	allocationSize = size; // Requested size
	allocationOffset = safe_static_cast<size_t>(physicalCurrent_ - virtualStart_);
	oldCurrent = physicalCurrent_;
	size += sizeof(BlockHeader); // add room for our book keeping.

	// Get aligned location.
	physicalCurrent_ = pointerUtil::AlignTop<char>(
					&physicalCurrent_[offset + sizeof(BlockHeader)], alignment)
					- (offset + sizeof(BlockHeader));

	// space?
	if (&physicalCurrent_[size] > physicalEnd_)
	{
		neededPhysicalSize = bitUtil::RoundUpToMultiple(size, granularity_);

		if (&physicalEnd_[neededPhysicalSize] <= virtualEnd_)
		{
			VirtualMem::AllocatePhysicalMemory(physicalEnd_, neededPhysicalSize);
			physicalEnd_ += neededPhysicalSize;
		}
	}

#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
		++statistics_.allocationCount_;
		statistics_.allocationCountMax_ = core::Max<size_t>(statistics_.allocationCount_, statistics_.allocationCountMax_);
		statistics_.physicalMemoryAllocated_ = safe_static_cast<size_t>(physicalEnd_ - virtualStart_);
		statistics_.physicalMemoryAllocatedMax_ = core::Max<size_t>(statistics_.physicalMemoryAllocated_, statistics_.physicalMemoryAllocatedMax_);
		statistics_.physicalMemoryUsed_ = safe_static_cast<size_t>(physicalCurrent_ - virtualStart_);
		statistics_.physicalMemoryUsedMax_ = core::Max<size_t>(statistics_.physicalMemoryUsed_, statistics_.physicalMemoryUsedMax_);
		statistics_.wasteAlignment_ += safe_static_cast<size_t>((uintptr_t)(physicalCurrent_ + (uintptr_t)oldCurrent - size));
		statistics_.wasteAlignmentMax_ = core::Max<size_t>(statistics_.wasteAlignment_, statistics_.wasteAlignmentMax_);
		statistics_.internalOverhead_ += sizeof(BlockHeader);
		statistics_.internalOverheadMax_ = core::Max<size_t>(statistics_.internalOverhead_, statistics_.internalOverheadMax_);
#endif
	

	union
	{
		void* as_void;
		char* as_char;
		BlockHeader* as_header;
	};

	as_char = physicalCurrent_;
#if X_ENABLE_STACK_ALLOCATOR_CHECK
	as_header->AllocationID_ = allocationID_++;
#endif
	as_header->allocationOffset_ = safe_static_cast<uint32_t>(allocationOffset);
	as_header->allocationSize_ = safe_static_cast<uint32_t>(allocationSize);
	as_char += sizeof(BlockHeader);

	void* userPtr = as_void;
	physicalCurrent_ += size;
	return userPtr;
}

void GrowingStackAllocator::free(void* ptr)
{
	X_ASSERT_NOT_NULL(ptr);

	union
	{
		void* as_void;
		char* as_char;
		BlockHeader* as_header;
	};

	as_void = ptr;
	as_char -= sizeof(BlockHeader);

#if X_ENABLE_STACK_ALLOCATOR_CHECK
	if (as_header->AllocationID_ != (allocationID_ - 1))
	{
		uint32_t AllocationID = as_header->AllocationID_;
		X_ASSERT(false, "Cannot free memory from stack(LIFO). invalid order.")(allocationID_, AllocationID, ptr);
	}

	allocationID_--;
#endif

	physicalCurrent_ = &virtualStart_[as_header->allocationOffset_];


#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
	statistics_.allocationCount_--;
	statistics_.physicalMemoryUsed_ = as_header->allocationOffset_;
	statistics_.internalOverhead_ -= sizeof(BlockHeader);
	statistics_.wasteAlignment_ -= safe_static_cast<size_t>(as_char - physicalCurrent_);
#endif
}

void GrowingStackAllocator::free(void* ptr, size_t size)
{
	X_UNUSED(size);
	free(ptr);
}

void GrowingStackAllocator::purge(void)
{
	char* start = pointerUtil::AlignTop<char>(physicalCurrent_, granularity_);
	size_t size = safe_static_cast<size_t>(physicalEnd_ - start);

	VirtualMem::FreePhysicalMemory(start,size);

	physicalEnd_ = start;

#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
	statistics_.physicalMemoryAllocated_ = safe_static_cast<size_t>(physicalEnd_ - virtualStart_);
#endif
}


MemoryAllocatorStatistics GrowingStackAllocator::getStatistics(void) const
{
#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
	return statistics_;
#else
	MemoryAllocatorStatistics stats;
	core::zero_object(stats);
	return stats;
#endif
}

X_NAMESPACE_END