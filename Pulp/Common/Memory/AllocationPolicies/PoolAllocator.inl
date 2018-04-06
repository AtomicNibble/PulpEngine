
// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
X_INLINE void* PoolAllocator::allocate(size_t size, size_t alignment, size_t offset)
{
    X_ASSERT(size <= maxSize_, "A pool allocator can only allocate instances of the same or smaller size.")(maxSize_, size, alignment, offset);

#if X_ENABLE_POOL_ALLOCATOR_CHECK
    X_ASSERT(alignment <= maxAlignment_, "A pool allocator can only allocate instances with the same or smaller alignment.")(maxSize_, size, maxAlignment_, alignment, offset_, offset);
    X_ASSERT(offset == offset_, "A pool allocator can only allocate instances with the same offset.")(maxSize_, size, maxAlignment_, alignment, offset_, offset);
#else
    X_UNUSED(alignment);
    X_UNUSED(offset);
#endif

    void* memory = freelist_.Obtain();

    X_ASSERT(memory != nullptr, "failed to allocate from pool")();

#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
    if (memory != nullptr) {
        ++statistics_.allocationCount_;
        statistics_.allocationCountMax_ = Max(statistics_.allocationCount_, statistics_.allocationCountMax_);

        statistics_.physicalMemoryUsed_ += elementSize_;
        statistics_.physicalMemoryUsedMax_ = Max(statistics_.physicalMemoryUsed_, statistics_.physicalMemoryUsedMax_);

        statistics_.wasteAlignment_ += wastePerElement_;
        statistics_.wasteAlignmentMax_ = Max(statistics_.wasteAlignment_, statistics_.wasteAlignmentMax_);
    }
#endif

    return memory;
}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
X_INLINE void PoolAllocator::free(void* ptr)
{
    X_ASSERT_NOT_NULL(ptr);

    freelist_.Return(ptr);

#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
    --statistics_.allocationCount_;
    statistics_.allocationCountMax_ = Max(statistics_.allocationCount_, statistics_.allocationCountMax_);

    statistics_.physicalMemoryUsed_ -= elementSize_;
    statistics_.physicalMemoryUsedMax_ = Max(statistics_.physicalMemoryUsed_, statistics_.physicalMemoryUsedMax_);

    statistics_.wasteAlignment_ -= wastePerElement_;
    statistics_.wasteAlignmentMax_ = Max(statistics_.wasteAlignment_, statistics_.wasteAlignmentMax_);
#endif
}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
X_INLINE void PoolAllocator::free(void* ptr, size_t size)
{
    X_UNUSED(size);

    free(ptr);
}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
X_INLINE size_t PoolAllocator::getSize(void*) const
{
    return maxSize_;
}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
X_INLINE size_t PoolAllocator::usableSize(void*) const
{
    return maxSize_;
}