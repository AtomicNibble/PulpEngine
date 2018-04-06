

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
inline size_t MallocFreeAllocator::getSize(void* allocation) const
{
    X_ASSERT_NOT_NULL(allocation);

    union
    {
        void* as_void;
        BlockHeader* as_header;
    };

    as_void = allocation;
    return as_header[-1].originalSize_;
}

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
inline size_t MallocFreeAllocator::usableSize(void* ptr) const
{
    return getSize(ptr);
}