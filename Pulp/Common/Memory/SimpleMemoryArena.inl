

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <class AllocationPolicy>
SimpleMemoryArena<AllocationPolicy>::SimpleMemoryArena(AllocationPolicy* allocator, const char* name) :
	allocator_(allocator),
	name_(name)
{
#if X_ENABLE_MEMORY_ARENA_STATISTICS
	statistics_.arenaName_ = name;
	statistics_.arenaType_ = "SimpleMemoryArena";
	statistics_.threadPolicyType_ = "none";
	statistics_.boundsCheckingPolicyType_ = "none";
	statistics_.memoryTrackingPolicyType_ = "none";
	statistics_.memoryTaggingPolicyType_ = "none";
	statistics_.allocatorStatistics_ = allocator->getStatistics();
	statistics_.trackingOverhead_ = 0;
	statistics_.boundsCheckingOverhead_ = 0;
#endif
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <class AllocationPolicy>
SimpleMemoryArena<AllocationPolicy>::~SimpleMemoryArena(void)
{
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <class AllocationPolicy>
void* SimpleMemoryArena<AllocationPolicy>::allocate(size_t size, size_t alignment, size_t offset, const char*, const char* X_SOURCE_INFO_MEM_CB(const SourceInfo&))
{
	void* memory = allocator_->allocate(size, alignment, offset);
	X_ASSERT(memory != nullptr, "Out of memory. Cannot allocate %d bytes from arena \"%s\".", size, name_)(size, alignment, offset);

#if X_ENABLE_MEMORY_ARENA_STATISTICS
	statistics_.allocatorStatistics_ = allocator_->getStatistics();
#endif

	return memory;
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <class AllocationPolicy>
void SimpleMemoryArena<AllocationPolicy>::free(void* ptr)
{
	allocator_->free(X_ASSERT_NOT_NULL(ptr));

#if X_ENABLE_MEMORY_ARENA_STATISTICS
	statistics_.allocatorStatistics_ = allocator_->getStatistics();
#endif
}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <class AllocationPolicy>
void SimpleMemoryArena<AllocationPolicy>::free(void* ptr, size_t size)
{
	allocator_->free(X_ASSERT_NOT_NULL(ptr), size);

#if X_ENABLE_MEMORY_ARENA_STATISTICS
	statistics_.allocatorStatistics_ = allocator_->getStatistics();
#endif
}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <class AllocationPolicy>
size_t SimpleMemoryArena<AllocationPolicy>::getSize(void* ptr)
{
	return allocator_->getSize(ptr);
}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <class AllocationPolicy>
MemoryArenaStatistics SimpleMemoryArena<AllocationPolicy>::getStatistics(void) const
{
#if X_ENABLE_MEMORY_ARENA_STATISTICS
	return statistics_;
#else
	MemoryArenaStatistics statistics = {};
	statistics.arenaName_ = name_;
	statistics.arenaType_ = "SimpleMemoryArena";
	statistics.threadPolicyType_ = "none";
	statistics.boundsCheckingPolicyType_ = "none";
	statistics.memoryTrackingPolicyType_ = "none";
	statistics.memoryTaggingPolicyType_ = "none";
	statistics.allocatorStatistics_ = allocator_->getStatistics();
	statistics.trackingOverhead_ = 0;
	statistics.boundsCheckingOverhead_ = 0;
	return statistics;
#endif
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <class AllocationPolicy>
MemoryAllocatorStatistics SimpleMemoryArena<AllocationPolicy>::getAllocatorStatistics(bool children) const
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

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------

template <class AllocationPolicy>
bool SimpleMemoryArena<AllocationPolicy>::isThreadSafe(void) const
{
	return false;
}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <class AllocationPolicy>
inline size_t SimpleMemoryArena<AllocationPolicy>::getMemoryRequirement(size_t size)
{
	return size;
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <class AllocationPolicy>
inline size_t SimpleMemoryArena<AllocationPolicy>::getMemoryAlignmentRequirement(size_t alignment)
{
	return alignment;
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <class AllocationPolicy>
inline size_t SimpleMemoryArena<AllocationPolicy>::getMemoryOffsetRequirement(void)
{
	return 0;
}
