#include "EngineCommon.h"

#include "GrowingGenericAllocator.h"

X_NAMESPACE_BEGIN(core)

GrowingGenericAllocator::GrowingGenericAllocator(uint32_t maxSizeInBytesPerPool, uint32_t growSize, size_t maxAlignment, size_t offset) :
    microAllocator_(maxSizeInBytesPerPool, growSize, maxAlignment, offset),
    blockAllocator_()
{
#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
    zero_object(statistics_);

    statistics_.type_ = "GrowingGeneric";
#endif // X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
}

MemoryAllocatorStatistics GrowingGenericAllocator::getStatistics(void) const
{
#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
    return statistics_;
#else // X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
    static MemoryAllocatorStatistics stats;
    core::zero_object(stats);
    return stats;
#endif // X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
}

#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
void GrowingGenericAllocator::updateStatistics(void)
{
    MemoryAllocatorStatistics Micro = microAllocator_.getStatistics();
    MemoryAllocatorStatistics Block = blockAllocator_.getStatistics();

    statistics_.Clear();
    statistics_ += Micro;
    statistics_ += Block;
}
#endif // X_ENABLE_MEMORY_ALLOCATOR_STATISTICS

X_NAMESPACE_END
