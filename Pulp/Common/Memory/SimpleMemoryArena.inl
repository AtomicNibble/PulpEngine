

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <class AllocationPolicy>
SimpleMemoryArena<AllocationPolicy>::SimpleMemoryArena(AllocationPolicy* allocator, const char* name)
	: m_allocator(allocator)
	, m_name(name)
{
#if X_ENABLE_MEMORY_ARENA_STATISTICS
	m_statistics.m_arenaName = name;
	m_statistics.m_arenaType = "SimpleMemoryArena";
	m_statistics.m_threadPolicyType = "none";
	m_statistics.m_boundsCheckingPolicyType = "none";
	m_statistics.m_memoryTrackingPolicyType = "none";
	m_statistics.m_memoryTaggingPolicyType = "none";
	m_statistics.m_allocatorStatistics = allocator->GetStatistics();
	m_statistics.m_trackingOverhead = 0;
	m_statistics.m_boundsCheckingOverhead = 0;
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
void* SimpleMemoryArena<AllocationPolicy>::Allocate(size_t size, size_t alignment, size_t offset, const char*, const char*, const SourceInfo&)
{
	void* memory = m_allocator->Allocate(size, alignment, offset);
	X_ASSERT(memory != nullptr, "Out of memory. Cannot allocate %d bytes from arena \"%s\".", size, m_name)(size, alignment, offset);

#if X_ENABLE_MEMORY_ARENA_STATISTICS
	m_statistics.m_allocatorStatistics = m_allocator->GetStatistics();
#endif

	return memory;
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <class AllocationPolicy>
void SimpleMemoryArena<AllocationPolicy>::Free(void* ptr)
{
	X_ASSERT_NOT_NULL(ptr);

	m_allocator->Free(ptr);

#if X_ENABLE_MEMORY_ARENA_STATISTICS
	m_statistics.m_allocatorStatistics = m_allocator->GetStatistics();
#endif
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <class AllocationPolicy>
MemoryArenaStatistics SimpleMemoryArena<AllocationPolicy>::GetStatistics(void) const
{
#if X_ENABLE_MEMORY_ARENA_STATISTICS
	return m_statistics;
#else
	MemoryArenaStatistics statistics = {};
	statistics.m_arenaName = m_name;
	statistics.m_arenaType = "SimpleMemoryArena";
	statistics.m_threadPolicyType = "none";
	statistics.m_boundsCheckingPolicyType = "none";
	statistics.m_memoryTrackingPolicyType = "none";
	statistics.m_memoryTaggingPolicyType = "none";
	statistics.m_allocatorStatistics = m_allocator->GetStatistics();
	statistics.m_trackingOverhead = 0;
	statistics.m_boundsCheckingOverhead = 0;
	return statistics;
#endif
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <class AllocationPolicy>
inline size_t SimpleMemoryArena<AllocationPolicy>::GetMemoryRequirement(size_t size)
{
	return size;
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <class AllocationPolicy>
inline size_t SimpleMemoryArena<AllocationPolicy>::GetMemoryAlignmentRequirement(size_t alignment)
{
	return alignment;
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
template <class AllocationPolicy>
inline size_t SimpleMemoryArena<AllocationPolicy>::GetMemoryOffsetRequirement(void)
{
	return 0;
}
