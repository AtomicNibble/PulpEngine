

// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
inline size_t GrowingBlockAllocator::getSize(void* ptr) const
{
    X_ASSERT_NOT_NULL(ptr);

    union
    {
        void* as_void;
        BlockHeader* as_header;
    };

    as_void = ptr;
    return as_header[-1].originalSize_;
}
