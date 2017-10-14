
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
X_INLINE size_t GrowingMicroAllocator::usableSize(void* ptr) const
{
	if (poolAllocator8_.containsAllocation(ptr))
		return poolAllocator8_.usableSize(ptr);
	else if (poolAllocator16_.containsAllocation(ptr))
		return poolAllocator16_.usableSize(ptr);
	else if (poolAllocator32_.containsAllocation(ptr))
		return poolAllocator32_.usableSize(ptr);
	else if (poolAllocator64_.containsAllocation(ptr))
		return poolAllocator64_.usableSize(ptr);
	else if (poolAllocator128_.containsAllocation(ptr))
		return poolAllocator128_.usableSize(ptr);
	else if (poolAllocator256_.containsAllocation(ptr))
		return poolAllocator256_.usableSize(ptr);

	X_ASSERT(false, "Cannot determine usableSize of allocation. Allocation does not belong to this allocator.")(ptr);
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
X_INLINE bool GrowingMicroAllocator::containsAllocation(void* ptr, size_t size) const
{
	X_UNUSED(ptr);

#if X_ENABLE_ASSERTIONS
	if (size <= MAX_ALLOCATION_SIZE) {
		X_ASSERT(containsAllocation(ptr), "Allocation that fits in pool did not come from pool")(ptr, size);
	}
#endif

	return size <= MAX_ALLOCATION_SIZE;
}
// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
X_INLINE bool GrowingMicroAllocator::containsAllocation(void* ptr, size_t* pSizeOut) const
{
	if (poolAllocator8_.containsAllocation(ptr)) {
		*pSizeOut = poolAllocator8_.getSize(ptr);
	}
	else if (poolAllocator16_.containsAllocation(ptr)) {
		*pSizeOut = poolAllocator16_.getSize(ptr);
	}
	else if (poolAllocator32_.containsAllocation(ptr)) {
		*pSizeOut = poolAllocator32_.getSize(ptr);
	}
	else if (poolAllocator64_.containsAllocation(ptr)) {
		*pSizeOut = poolAllocator64_.getSize(ptr);
	}
	else if (poolAllocator128_.containsAllocation(ptr)) {
		*pSizeOut = poolAllocator128_.getSize(ptr);
	}
	else if (poolAllocator256_.containsAllocation(ptr)) {
		*pSizeOut = poolAllocator256_.getSize(ptr);
	}
	else
	{
		return false;
	}

	return true;
}
