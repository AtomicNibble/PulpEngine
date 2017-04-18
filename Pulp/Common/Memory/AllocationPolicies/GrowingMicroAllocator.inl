
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


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
X_INLINE bool GrowingMicroAllocator::containsAllocation(void* ptr, size_t& sizeOut) const
{
	if (poolAllocator8_.containsAllocation(ptr)) {
		sizeOut = poolAllocator8_.getSize(ptr);
	}
	else if (poolAllocator16_.containsAllocation(ptr)) {
		sizeOut = poolAllocator16_.getSize(ptr);
	}
	else if (poolAllocator32_.containsAllocation(ptr)) {
		sizeOut = poolAllocator32_.getSize(ptr);
	}
	else if (poolAllocator64_.containsAllocation(ptr)) {
		sizeOut = poolAllocator64_.getSize(ptr);
	}
	else if (poolAllocator128_.containsAllocation(ptr)) {
		sizeOut = poolAllocator128_.getSize(ptr);
	}
	else if (poolAllocator256_.containsAllocation(ptr)) {
		sizeOut = poolAllocator256_.getSize(ptr);
	}
	else
	{
		return false;
	}

	return true;
}
