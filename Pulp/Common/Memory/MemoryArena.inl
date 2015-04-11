
// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <class AllocationPolicy, class ThreadPolicy, class BoundsCheckingPolicy, class MemoryTrackingPolicy, class MemoryTaggingPolicy>
MemoryArena<AllocationPolicy, ThreadPolicy, BoundsCheckingPolicy, MemoryTrackingPolicy, MemoryTaggingPolicy>::MemoryArena(AllocationPolicy* allocator, const char* name)
	: m_allocator(allocator)
	, m_name(name)
	, m_isFrozen(false)
{
#if X_ENABLE_MEMORY_ARENA_STATISTICS
	m_statistics.m_arenaName = name;
	m_statistics.m_arenaType = "MemoryArena";
	m_statistics.m_threadPolicyType = ThreadPolicy::TYPE_NAME;
	m_statistics.m_boundsCheckingPolicyType = BoundsCheckingPolicy::TYPE_NAME;
	m_statistics.m_memoryTrackingPolicyType = MemoryTrackingPolicy::TYPE_NAME;
	m_statistics.m_memoryTaggingPolicyType = MemoryTaggingPolicy::TYPE_NAME;
	m_statistics.m_allocatorStatistics = allocator->getStatistics();
	m_statistics.m_trackingOverhead = 0;
	m_statistics.m_boundsCheckingOverhead = 0;
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
void* MemoryArena<AllocationPolicy, ThreadPolicy, BoundsCheckingPolicy, MemoryTrackingPolicy, MemoryTaggingPolicy>::allocate(size_t size, size_t alignment, size_t offset, const char* ID, const char* typeName, const SourceInfo& sourceInfo)
{
	X_ASSERT(bitUtil::IsPowerOfTwo(alignment), "Alignment is not a power-of-two.")(size, alignment, offset);
	X_ASSERT((offset % 4) == 0, "Offset is not a multiple of 4.")(size, alignment, offset);
	X_ASSERT(!m_isFrozen, "Memory arena \"%s\" is frozen, no memory can be allocated.", m_name)();

	m_threadGuard.Enter();

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
	as_void = m_allocator->allocate(newSize, alignment, offset + overheadFront);

	X_ASSERT(as_void != nullptr, "Out of memory. Cannot allocate %d bytes from arena \"%s\".", newSize, m_name)(size, newSize, alignment, offset, overheadFront, overheadBack);

	const size_t RealAllocSize = m_allocator->getSize(as_void);

	// then tell the memory tracker about the allocation
	m_memoryTracker.OnAllocation(as_void, size, newSize, alignment, offset, ID, typeName, sourceInfo, m_name);

	// the first few bytes belong to the bounds checker
	m_boundsChecker.GuardFront(as_void);
	as_char += BoundsCheckingPolicy::SIZE_FRONT;

	// tag the allocation, and store the pointer which is later handed to the user
	m_memoryTagger.TagAllocation(as_void, RealAllocSize - overheadTotal );

	void* userPtr = as_void;

	as_char += (RealAllocSize - overheadTotal);

	// the last few bytes belong to the bounds checker as well
	// we place at end of allocation.
	m_boundsChecker.GuardBack(as_void);

#if X_ENABLE_MEMORY_ARENA_STATISTICS
	m_statistics.m_allocatorStatistics = m_allocator->getStatistics();
	m_statistics.m_trackingOverhead += MemoryTrackingPolicy::PER_ALLOCATION_OVERHEAD;
	m_statistics.m_boundsCheckingOverhead += BoundsCheckingPolicy::SIZE_FRONT + BoundsCheckingPolicy::SIZE_BACK;
#endif

	m_threadGuard.Leave();

	return userPtr;
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <class AllocationPolicy, class ThreadPolicy, class BoundsCheckingPolicy, class MemoryTrackingPolicy, class MemoryTaggingPolicy>
void MemoryArena<AllocationPolicy, ThreadPolicy, BoundsCheckingPolicy, MemoryTrackingPolicy, MemoryTaggingPolicy>::free(void* ptr)
{
	X_ASSERT_NOT_NULL(ptr);
	X_ASSERT(!m_isFrozen, "Memory arena \"%s\" is frozen, no memory can be freed.", m_name)();

	if (ptr)
	{
		m_threadGuard.Enter();

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
		const size_t allocationSize = m_allocator->getSize(originalRawMemory);
		char* backGuard = frontGuard + allocationSize - BoundsCheckingPolicy::SIZE_BACK;

		m_boundsChecker.CheckFront(frontGuard);
		m_boundsChecker.CheckBack(backGuard);
		m_memoryTracker.OnDeallocation(originalRawMemory);
		m_memoryTagger.TagDeallocation(ptr, allocationSize - BoundsCheckingPolicy::SIZE_FRONT - BoundsCheckingPolicy::SIZE_BACK);

		m_allocator->free(originalRawMemory);

#if X_ENABLE_MEMORY_ARENA_STATISTICS
		m_statistics.m_allocatorStatistics = m_allocator->getStatistics();
		m_statistics.m_trackingOverhead -= MemoryTrackingPolicy::PER_ALLOCATION_OVERHEAD;
		m_statistics.m_boundsCheckingOverhead -= BoundsCheckingPolicy::SIZE_FRONT + BoundsCheckingPolicy::SIZE_BACK;
#endif

		m_threadGuard.Leave();
	}
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <class AllocationPolicy, class ThreadPolicy, class BoundsCheckingPolicy, class MemoryTrackingPolicy, class MemoryTaggingPolicy>
MemoryArenaStatistics MemoryArena<AllocationPolicy, ThreadPolicy, BoundsCheckingPolicy, MemoryTrackingPolicy, MemoryTaggingPolicy>::getStatistics(void) const
{
#if X_ENABLE_MEMORY_ARENA_STATISTICS
	return m_statistics;
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
	MemoryAllocatorStatistics stats = m_statistics.m_allocatorStatistics;

	if (children) {
		for (auto arena : children_)
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





// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <class AllocationPolicy, class ThreadPolicy, class BoundsCheckingPolicy, class MemoryTrackingPolicy, class MemoryTaggingPolicy>
inline void MemoryArena<AllocationPolicy, ThreadPolicy, BoundsCheckingPolicy, MemoryTrackingPolicy, MemoryTaggingPolicy>::freeze(void)
{
	m_isFrozen = true;
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <class AllocationPolicy, class ThreadPolicy, class BoundsCheckingPolicy, class MemoryTrackingPolicy, class MemoryTaggingPolicy>
inline void MemoryArena<AllocationPolicy, ThreadPolicy, BoundsCheckingPolicy, MemoryTrackingPolicy, MemoryTaggingPolicy>::unfreeze(void)
{
	m_isFrozen = false;
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
