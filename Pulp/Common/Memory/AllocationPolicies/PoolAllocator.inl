
// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
X_INLINE void* PoolAllocator::allocate(size_t size, size_t alignment, size_t offset)
{
	X_ASSERT(size <= m_maxSize, "A pool allocator can only allocate instances of the same or smaller size.")(m_maxSize, size, alignment, offset);

#if X_ENABLE_POOL_ALLOCATOR_CHECK
	X_ASSERT(alignment <= m_maxAlignment, "A pool allocator can only allocate instances with the same or smaller alignment.")(m_maxSize, size, m_maxAlignment, alignment, m_offset, offset);
	X_ASSERT(offset == m_offset, "A pool allocator can only allocate instances with the same offset.")(m_maxSize, size, m_maxAlignment, alignment, m_offset, offset);
#else
	X_UNUSED(alignment);
	X_UNUSED(offset);
#endif

	void* memory = m_freelist.Obtain();

	X_ASSERT(memory != nullptr, "failed to allocate from pool")();

#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
	if (memory != nullptr)
	{
		++m_statistics.m_allocationCount;
		m_statistics.m_allocationCountMax = Max(m_statistics.m_allocationCount, m_statistics.m_allocationCountMax);

		m_statistics.m_physicalMemoryUsed += m_elementSize;
		m_statistics.m_physicalMemoryUsedMax = Max(m_statistics.m_physicalMemoryUsed, m_statistics.m_physicalMemoryUsedMax);

		m_statistics.m_wasteAlignment += m_wastePerElement;
		m_statistics.m_wasteAlignmentMax = Max(m_statistics.m_wasteAlignment, m_statistics.m_wasteAlignmentMax);
	}
#endif

	return memory;
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
X_INLINE void PoolAllocator::free(void* ptr)
{
	X_ASSERT_NOT_NULL(ptr);

	m_freelist.Return(ptr);

#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
	--m_statistics.m_allocationCount;
	m_statistics.m_allocationCountMax = Max(m_statistics.m_allocationCount, m_statistics.m_allocationCountMax);

	m_statistics.m_physicalMemoryUsed -= m_elementSize;
	m_statistics.m_physicalMemoryUsedMax = Max(m_statistics.m_physicalMemoryUsed, m_statistics.m_physicalMemoryUsedMax);

	m_statistics.m_wasteAlignment -= m_wastePerElement;
	m_statistics.m_wasteAlignmentMax = Max(m_statistics.m_wasteAlignment, m_statistics.m_wasteAlignmentMax);
#endif
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
X_INLINE size_t PoolAllocator::getSize(void*) const
{
	return m_maxSize;
}
