
// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
X_INLINE void* GrowingGenericAllocator::allocate(size_t size, size_t alignment, size_t offset)
{
	// defer small allocations to the micro allocator, and use the block allocator for the rest
	if (size <= GrowingMicroAllocator::MAX_ALLOCATION_SIZE)
	{
		void* memory = m_microAllocator.allocate(size, alignment, offset);

#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
		updateStatistics();
#endif

		return memory;
	}
	else
	{
		void* memory = m_blockAllocator.allocate(size, alignment, offset);

#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
		updateStatistics();
#endif

		return memory;
	}
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
X_INLINE void GrowingGenericAllocator::free(void* ptr)
{
	// we need to figure out which allocator made the allocation in order to free it. asking the micro allocator
	// whether it contains an allocation is an O(1) operation. if the allocation isn't contained in the micro
	// allocator, we assume that it was done by the block allocator, and try to free it accordingly.
	if (m_microAllocator.containsAllocation(ptr))
	{
		m_microAllocator.free(ptr);
	}
	else
	{
		m_blockAllocator.free(ptr);
	}

#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
	updateStatistics();
#endif
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
X_INLINE size_t GrowingGenericAllocator::getSize(void* ptr) const
{
	if (m_microAllocator.containsAllocation(ptr))
	{
		return m_microAllocator.getSize(ptr);
	}
	else
	{
		return m_blockAllocator.getSize(ptr);
	}
}
