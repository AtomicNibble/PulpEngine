
// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
X_INLINE size_t GrowingMicroAllocator::getSize(void* ptr) const
{
	if (m_poolAllocator8.containsAllocation(ptr))
		return m_poolAllocator8.getSize(ptr);
	else if (m_poolAllocator16.containsAllocation(ptr))
		return m_poolAllocator16.getSize(ptr);
	else if (m_poolAllocator32.containsAllocation(ptr))
		return m_poolAllocator32.getSize(ptr);
	else if (m_poolAllocator64.containsAllocation(ptr))
		return m_poolAllocator64.getSize(ptr);
	else if (m_poolAllocator128.containsAllocation(ptr))
		return m_poolAllocator128.getSize(ptr);
	else if (m_poolAllocator256.containsAllocation(ptr))
		return m_poolAllocator256.getSize(ptr);

	X_ASSERT(false, "Cannot determine size of allocation. Allocation does not belong to this allocator.")(ptr);
	return 0;
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
X_INLINE bool GrowingMicroAllocator::containsAllocation(void* ptr) const
{
	return (m_poolAllocator8.containsAllocation(ptr) ||
		m_poolAllocator16.containsAllocation(ptr) ||
		m_poolAllocator32.containsAllocation(ptr) ||
		m_poolAllocator64.containsAllocation(ptr) ||
		m_poolAllocator128.containsAllocation(ptr) ||
		m_poolAllocator256.containsAllocation(ptr));
}
