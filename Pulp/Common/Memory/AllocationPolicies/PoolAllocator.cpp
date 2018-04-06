#include "EngineCommon.h"

#include "PoolAllocator.h"
#include "Util/BitUtil.h"
#include "Util/PointerUtil.h"

X_NAMESPACE_BEGIN(core)

namespace
{
    size_t CalculateElementSize(size_t maxElementSize, size_t maxAlignment)
    {
        return bitUtil::RoundUpToMultiple(maxElementSize, maxAlignment);
    }

    size_t CalculateWasteAtFront(void* start, size_t maxAlignment, size_t offset)
    {
        char* pAdd = pointerUtil::AlignTop((char*)start + offset, maxAlignment);

        return safe_static_cast<size_t>((pAdd - offset) - (char*)start);
    }

} // namespace

PoolAllocator::PoolAllocator(void* start, void* end, size_t maxElementSize, size_t maxAlignment, size_t offset) :
    freelist_(start, end, maxElementSize, maxAlignment, offset),
    maxSize_(maxElementSize)

#if X_ENABLE_POOL_ALLOCATOR_CHECK
    ,
    maxAlignment_(maxAlignment),
    offset_(offset)
#endif
{
#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
    size_t ElemSize = CalculateElementSize(maxElementSize, maxAlignment);
    size_t Waste = CalculateWasteAtFront(start, maxAlignment, offset);
    size_t Size = safe_static_cast<size_t>((char*)end - (char*)start);

    elementSize_ = ElemSize;
    wastePerElement_ = ElemSize - maxElementSize;

    zero_object(statistics_);

    statistics_.type_ = "Pool";
    statistics_.virtualMemoryReserved_ = Size;
    statistics_.physicalMemoryAllocated_ = Size;
    statistics_.physicalMemoryUsed_ = Waste;
    statistics_.wasteAlignment_ = Waste;

    statistics_.wasteUnused_ = (Size - Waste) - (elementSize_ * ((Size - Waste) / elementSize_));

    statistics_.wasteAlignmentMax_ = Max(statistics_.wasteAlignment_, statistics_.wasteAlignmentMax_);
    statistics_.wasteUnusedMax_ = Max(statistics_.wasteUnused_, statistics_.wasteUnusedMax_);
    statistics_.allocationCountMax_ = Max(statistics_.allocationCount_, statistics_.allocationCountMax_);
    statistics_.internalOverheadMax_ = Max(statistics_.internalOverhead_, statistics_.internalOverheadMax_);
    statistics_.physicalMemoryUsedMax_ = Max(statistics_.physicalMemoryUsed_, statistics_.physicalMemoryUsedMax_);
    statistics_.physicalMemoryAllocatedMax_ = Max(statistics_.physicalMemoryAllocated_, statistics_.physicalMemoryAllocatedMax_);
#endif
}

MemoryAllocatorStatistics PoolAllocator::getStatistics(void) const
{
#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
    return this->statistics_;
#else
    static MemoryAllocatorStatistics stats;
    core::zero_object(stats);
    return stats;
#endif
}

X_NAMESPACE_END