
// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
X_INLINE void* GrowingGenericAllocator::allocate(size_t size, size_t alignment, size_t offset)
{
    // defer small allocations to the micro allocator, and use the block allocator for the rest
    if (size <= GrowingMicroAllocator::MAX_ALLOCATION_SIZE) {
        void* memory = microAllocator_.allocate(size, alignment, offset);

#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
        updateStatistics();
#endif

        return memory;
    }
    else {
        void* memory = blockAllocator_.allocate(size, alignment, offset);

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
    if (microAllocator_.containsAllocation(ptr)) {
        microAllocator_.free(ptr);
    }
    else {
        blockAllocator_.free(ptr);
    }

#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
    updateStatistics();
#endif
}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
X_INLINE void GrowingGenericAllocator::free(void* ptr, size_t size)
{
    if (microAllocator_.containsAllocation(ptr, size)) {
        microAllocator_.free(ptr, size);
    }
    else {
        blockAllocator_.free(ptr, size);
    }

#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
    updateStatistics();
#endif
}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
X_INLINE size_t GrowingGenericAllocator::getSize(void* ptr) const
{
    size_t size;
    if (microAllocator_.containsAllocation(ptr, &size)) {
        return size;
    }
    else {
        return blockAllocator_.getSize(ptr);
    }
}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
X_INLINE size_t GrowingGenericAllocator::usableSize(void* ptr) const
{
    if (microAllocator_.containsAllocation(ptr)) {
        return microAllocator_.usableSize(ptr);
    }
    else {
        return blockAllocator_.usableSize(ptr);
    }
}
