
// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <class AllocationPolicy, class ThreadPolicy, class BoundsCheckingPolicy, class MemoryTrackingPolicy, class MemoryTaggingPolicy>
MemoryArena<AllocationPolicy, ThreadPolicy, BoundsCheckingPolicy, MemoryTrackingPolicy, MemoryTaggingPolicy>::MemoryArena(AllocationPolicy* allocator, const char* name) :
	allocator_(allocator),
	name_(name),
	isFrozen_(false)
{
#if X_ENABLE_MEMORY_ARENA_STATISTICS
	statistics_.arenaName_ = name;
	statistics_.arenaType_ = "MemoryArena";
	statistics_.threadPolicyType_ = ThreadPolicy::TYPE_NAME;
	statistics_.boundsCheckingPolicyType_ = BoundsCheckingPolicy::TYPE_NAME;
	statistics_.memoryTrackingPolicyType_ = MemoryTrackingPolicy::TYPE_NAME;
	statistics_.memoryTaggingPolicyType_ = MemoryTaggingPolicy::TYPE_NAME;
	statistics_.allocatorStatistics_ = allocator->getStatistics();
	statistics_.trackingOverhead_ = 0;
	statistics_.boundsCheckingOverhead_ = 0;
#endif
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <class AllocationPolicy, class ThreadPolicy, class BoundsCheckingPolicy, class MemoryTrackingPolicy, class MemoryTaggingPolicy>
MemoryArena<AllocationPolicy, ThreadPolicy, BoundsCheckingPolicy, MemoryTrackingPolicy, MemoryTaggingPolicy>::~MemoryArena(void)
{
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <class AllocationPolicy, class ThreadPolicy, class BoundsCheckingPolicy, class MemoryTrackingPolicy, class MemoryTaggingPolicy>
void* MemoryArena<AllocationPolicy, ThreadPolicy, BoundsCheckingPolicy, MemoryTrackingPolicy, MemoryTaggingPolicy>::allocate(size_t size, size_t alignment, size_t offset, const char* ID, const char* typeName X_SOURCE_INFO_MEM_CB(const SourceInfo& sourceInfo))
{
	X_ASSERT(bitUtil::IsPowerOfTwo(alignment), "Alignment is not a power-of-two.")(size, alignment, offset);
	X_ASSERT((offset % 4) == 0, "Offset is not a multiple of 4.")(size, alignment, offset);
	X_ASSERT(!isFrozen_, "Memory arena \"%s\" is frozen, no memory can be allocated.", name_)();

	threadGuard_.Enter();

	union
	{
		void* as_void;
		char* as_char;
		size_t* as_size_t;
	};

	// we need to account for the overhead caused by the bounds checker
	const size_t overheadFront = BoundsCheckingPolicy::SIZE_FRONT;
	const size_t overheadBack = BoundsCheckingPolicy::SIZE_BACK;
	const size_t overheadTotal = overheadFront + overheadBack;
	const size_t newSize = size + overheadTotal;

	// allocate raw memory
	as_void = allocator_->allocate(newSize, alignment, offset + overheadFront);

	X_ASSERT(as_void != nullptr, "Out of memory. Cannot allocate %d bytes from arena \"%s\".", newSize, name_)(size, newSize, alignment, offset, overheadFront, overheadBack);

	const size_t RealAllocSize = allocator_->getSize(as_void);

	// then tell the memory tracker about the allocation
	memoryTracker_.OnAllocation(as_void, size, newSize, alignment, offset, ID, typeName X_SOURCE_INFO_MEM_CB(sourceInfo), name_);

	// the first few bytes belong to the bounds checker
	boundsChecker_.GuardFront(as_void);
	as_char += BoundsCheckingPolicy::SIZE_FRONT;

	// tag the allocation, and store the pointer which is later handed to the user
	memoryTagger_.TagAllocation(as_void, RealAllocSize - overheadTotal );

	void* userPtr = as_void;

	as_char += (RealAllocSize - overheadTotal);

	// the last few bytes belong to the bounds checker as well
	// we place at end of allocation.
	boundsChecker_.GuardBack(as_void);

#if X_ENABLE_MEMORY_ARENA_STATISTICS
	statistics_.allocatorStatistics_ = allocator_->getStatistics();
	statistics_.trackingOverhead_ += MemoryTrackingPolicy::PER_ALLOCATION_OVERHEAD;
	statistics_.boundsCheckingOverhead_ += BoundsCheckingPolicy::SIZE_FRONT + BoundsCheckingPolicy::SIZE_BACK;
#endif

	threadGuard_.Leave();

	return userPtr;
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <class AllocationPolicy, class ThreadPolicy, class BoundsCheckingPolicy, class MemoryTrackingPolicy, class MemoryTaggingPolicy>
void MemoryArena<AllocationPolicy, ThreadPolicy, BoundsCheckingPolicy, MemoryTrackingPolicy, MemoryTaggingPolicy>::free(void* ptr)
{
	X_ASSERT_NOT_NULL(ptr);
	X_ASSERT(!isFrozen_, "Memory arena \"%s\" is frozen, no memory can be freed.", name_)();

	threadGuard_.Enter();

	union
	{
		void* as_void;
		char* as_char;
	};

	// remember that the user allocation does not necessarily point to the original raw allocation.
	// bytes at front and at the back could be guard bytes used by the bounds checker.
	as_void = ptr;

	char* frontGuard = as_char - BoundsCheckingPolicy::SIZE_FRONT;
	char* originalRawMemory = frontGuard;
	const size_t allocationSize = allocator_->getSize(originalRawMemory);
	char* backGuard = frontGuard + allocationSize - BoundsCheckingPolicy::SIZE_BACK;

	boundsChecker_.CheckFront(frontGuard);
	boundsChecker_.CheckBack(backGuard);
	memoryTracker_.OnDeallocation(originalRawMemory);
	memoryTagger_.TagDeallocation(ptr, allocationSize - BoundsCheckingPolicy::SIZE_FRONT - BoundsCheckingPolicy::SIZE_BACK);

	allocator_->free(originalRawMemory);

#if X_ENABLE_MEMORY_ARENA_STATISTICS
	statistics_.allocatorStatistics_ = allocator_->getStatistics();
	statistics_.trackingOverhead_ -= MemoryTrackingPolicy::PER_ALLOCATION_OVERHEAD;
	statistics_.boundsCheckingOverhead_ -= BoundsCheckingPolicy::SIZE_FRONT + BoundsCheckingPolicy::SIZE_BACK;
#endif

	threadGuard_.Leave();
}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <class AllocationPolicy, class ThreadPolicy, class BoundsCheckingPolicy, class MemoryTrackingPolicy, class MemoryTaggingPolicy>
void MemoryArena<AllocationPolicy, ThreadPolicy, BoundsCheckingPolicy, MemoryTrackingPolicy, MemoryTaggingPolicy>::free(void* ptr, size_t size)
{
	X_ASSERT_NOT_NULL(ptr);
	X_ASSERT(!isFrozen_, "Memory arena \"%s\" is frozen, no memory can be freed.", name_)();

	threadGuard_.Enter();

	union
	{
		void* as_void;
		char* as_char;
	};

	// remember that the user allocation does not necessarily point to the original raw allocation.
	// bytes at front and at the back could be guard bytes used by the bounds checker.
	as_void = ptr;

	char* frontGuard = as_char - BoundsCheckingPolicy::SIZE_FRONT;
	char* originalRawMemory = frontGuard;
	const size_t allocationSize = allocator_->getSize(originalRawMemory);
	char* backGuard = frontGuard + allocationSize - BoundsCheckingPolicy::SIZE_BACK;

	boundsChecker_.CheckFront(frontGuard);
	boundsChecker_.CheckBack(backGuard);
	memoryTracker_.OnDeallocation(originalRawMemory);
	memoryTagger_.TagDeallocation(ptr, allocationSize - BoundsCheckingPolicy::SIZE_FRONT - BoundsCheckingPolicy::SIZE_BACK);

	allocator_->free(originalRawMemory, size);

#if X_ENABLE_MEMORY_ARENA_STATISTICS
	statistics_.allocatorStatistics_ = allocator_->getStatistics();
	statistics_.trackingOverhead_ -= MemoryTrackingPolicy::PER_ALLOCATION_OVERHEAD;
	statistics_.boundsCheckingOverhead_ -= BoundsCheckingPolicy::SIZE_FRONT + BoundsCheckingPolicy::SIZE_BACK;
#endif

	threadGuard_.Leave();
}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <class AllocationPolicy, class ThreadPolicy, class BoundsCheckingPolicy, class MemoryTrackingPolicy, class MemoryTaggingPolicy>
size_t MemoryArena<AllocationPolicy, ThreadPolicy, BoundsCheckingPolicy, MemoryTrackingPolicy, MemoryTaggingPolicy>::getSize(void* ptr)
{
	union
	{
		void* as_void;
		char* as_char;
	};

	// remember that the user allocation does not necessarily point to the original raw allocation.
	// bytes at front and at the back could be guard bytes used by the bounds checker.
	as_void = ptr;

	char* frontGuard = as_char - BoundsCheckingPolicy::SIZE_FRONT;
	char* originalRawMemory = frontGuard;
	const size_t allocationSize = allocator_->getSize(originalRawMemory);

	return allocationSize;
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <class AllocationPolicy, class ThreadPolicy, class BoundsCheckingPolicy, class MemoryTrackingPolicy, class MemoryTaggingPolicy>
MemoryArenaStatistics MemoryArena<AllocationPolicy, ThreadPolicy, BoundsCheckingPolicy, MemoryTrackingPolicy, MemoryTaggingPolicy>::getStatistics(void) const
{
#if X_ENABLE_MEMORY_ARENA_STATISTICS
	return statistics_;
#else
	MemoryArenaStatistics statistics;
	core::zero_object(statistics);
	return statistics;
#endif
}

template <class AllocationPolicy, class ThreadPolicy, class BoundsCheckingPolicy, class MemoryTrackingPolicy, class MemoryTaggingPolicy>
MemoryAllocatorStatistics MemoryArena<AllocationPolicy, ThreadPolicy, BoundsCheckingPolicy,
	MemoryTrackingPolicy, MemoryTaggingPolicy>::getAllocatorStatistics(bool children) const
{
#if X_ENABLE_MEMORY_ARENA_STATISTICS
	MemoryAllocatorStatistics stats = statistics_.allocatorStatistics_;

	if (children) 
	{
		for (const auto& arena : children_)
		{
			stats += arena->getAllocatorStatistics(true);
		}
	}

	return stats;
#else
	X_UNUSED(children);

	MemoryAllocatorStatistics statistics;
	core::zero_object(statistics);
	return statistics;
#endif
}

template <class AllocationPolicy, class ThreadPolicy, class BoundsCheckingPolicy, class MemoryTrackingPolicy, class MemoryTaggingPolicy>
bool MemoryArena<AllocationPolicy, ThreadPolicy, BoundsCheckingPolicy,
	MemoryTrackingPolicy, MemoryTaggingPolicy>::isThreadSafe(void) const
{
	return IS_THREAD_SAFE;
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <class AllocationPolicy, class ThreadPolicy, class BoundsCheckingPolicy, class MemoryTrackingPolicy, class MemoryTaggingPolicy>
inline void MemoryArena<AllocationPolicy, ThreadPolicy, BoundsCheckingPolicy, MemoryTrackingPolicy, MemoryTaggingPolicy>::freeze(void)
{
	isFrozen_ = true;
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <class AllocationPolicy, class ThreadPolicy, class BoundsCheckingPolicy, class MemoryTrackingPolicy, class MemoryTaggingPolicy>
inline void MemoryArena<AllocationPolicy, ThreadPolicy, BoundsCheckingPolicy, MemoryTrackingPolicy, MemoryTaggingPolicy>::unfreeze(void)
{
	isFrozen_ = false;
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <class AllocationPolicy, class ThreadPolicy, class BoundsCheckingPolicy, class MemoryTrackingPolicy, class MemoryTaggingPolicy>
inline size_t MemoryArena<AllocationPolicy, ThreadPolicy, BoundsCheckingPolicy, MemoryTrackingPolicy, MemoryTaggingPolicy>::getMemoryRequirement(size_t size)
{
	// we need to account for the overhead caused by the bounds checker
	return size + BoundsCheckingPolicy::SIZE_FRONT + BoundsCheckingPolicy::SIZE_BACK;
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <class AllocationPolicy, class ThreadPolicy, class BoundsCheckingPolicy, class MemoryTrackingPolicy, class MemoryTaggingPolicy>
inline size_t MemoryArena<AllocationPolicy, ThreadPolicy, BoundsCheckingPolicy, MemoryTrackingPolicy, MemoryTaggingPolicy>::getMemoryAlignmentRequirement(size_t alignment)
{
	return alignment;
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <class AllocationPolicy, class ThreadPolicy, class BoundsCheckingPolicy, class MemoryTrackingPolicy, class MemoryTaggingPolicy>
inline size_t MemoryArena<AllocationPolicy, ThreadPolicy, BoundsCheckingPolicy, MemoryTrackingPolicy, MemoryTaggingPolicy>::getMemoryOffsetRequirement(void)
{
	return BoundsCheckingPolicy::SIZE_FRONT;
}
