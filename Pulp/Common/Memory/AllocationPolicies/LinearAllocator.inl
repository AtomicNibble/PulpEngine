
// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
inline void LinearAllocator::free(void* ptr)
{
	X_ASSERT_NOT_NULL(ptr);
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
inline void LinearAllocator::reset(void)
{
#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
	m_statistics.m_allocationCount = 0;
	m_statistics.m_physicalMemoryUsed = 0;
	m_statistics.m_wasteAlignment = 0;
	m_statistics.m_internalOverhead = 0;
#endif

	m_current = m_start;
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
inline size_t LinearAllocator::getSize(void* allocation) const
{
	union
	{
		void* as_void;
		char* as_char;
		size_t* as_size;
	};

	// grab the allocation's size from the first N bytes before the user data
	as_void = allocation;
	as_char -= sizeof(size_t);
	return (*as_size);
}
