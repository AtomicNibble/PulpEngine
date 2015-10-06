
// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
X_INLINE size_t GrowingMicroAllocator::getSize(void* ptr) const
{
	if (poolAllocator8_.containsAllocation(ptr))
		return poolAllocator8_.getSize(ptr);
	else if (poolAllocator16_.containsAllocation(ptr))
		return poolAllocator16_.getSize(ptr);
	else if (poolAllocator32_.containsAllocation(ptr))
		return poolAllocator32_.getSize(ptr);
	else if (poolAllocator64_.containsAllocation(ptr))
		return poolAllocator64_.getSize(ptr);
	else if (poolAllocator128_.containsAllocation(ptr))
		return poolAllocator128_.getSize(ptr);
	else if (poolAllocator256_.containsAllocation(ptr))
		return poolAllocator256_.getSize(ptr);

	X_ASSERT(false, "Cannot determine size of allocation. Allocation does not belong to this allocator.")(ptr);
	return 0;
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
X_INLINE bool GrowingMicroAllocator::containsAllocation(void* ptr) const
{
	return (poolAllocator8_.containsAllocation(ptr) ||
		poolAllocator16_.containsAllocation(ptr) ||
		poolAllocator32_.containsAllocation(ptr) ||
		poolAllocator64_.containsAllocation(ptr) ||
		poolAllocator128_.containsAllocation(ptr) ||
		poolAllocator256_.containsAllocation(ptr));
}
