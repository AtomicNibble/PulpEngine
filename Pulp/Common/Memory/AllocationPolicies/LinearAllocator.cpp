#include "EngineCommon.h"

#include "LinearAllocator.h"
#include "Util\PointerUtil.h"

X_NAMESPACE_BEGIN(core)

LinearAllocator::LinearAllocator(void* start, void* end) :
    start_((char*)start),
    end_((char*)end),
    current_((char*)start)
{
#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
    zero_this(&statistics_);

    size_t Len = static_cast<size_t>(reinterpret_cast<uintptr_t>(end) - reinterpret_cast<uintptr_t>(start));

    statistics_.type_ = "Linear";
    statistics_.virtualMemoryReserved_ = Len;
    statistics_.physicalMemoryAllocated_ = Len;
    statistics_.physicalMemoryAllocatedMax_ = Len;
#endif
}

void* LinearAllocator::allocate(size_t size, size_t alignment, size_t align_offset)
{
#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
    // used to work out alignment waste.
    // const uint32_t allocationOffset = safe_static_cast<uint32_t>(current_ - start_);
    char* oldCurrent = current_;
#endif

    size += sizeof(size_t);         // add room for our book keeping.
    align_offset += sizeof(size_t); // offset includes base position.

    current_ = pointerUtil::AlignTop(current_ + align_offset, alignment) - align_offset;

    if ((current_ + size) > end_) { // check for overflow.
        X_ASSERT(false, "Stack overflow!, a linear allocator can't satisfy the request.")(size, end_ - start_, end_ - current_);
        return nullptr;
    }

#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
    // stats baby !
    statistics_.totalAllocations_++;
    statistics_.totalAllocationSize_ += size;
    statistics_.allocationCount_++;
    statistics_.internalOverhead_ += sizeof(size_t);
    statistics_.wasteAlignment_ += safe_static_cast<size_t>(reinterpret_cast<uintptr_t>(current_) - reinterpret_cast<uintptr_t>(oldCurrent));

#endif

    union
    {
        void* as_void;
        char* as_char;
        size_t* as_size_t;
    };

    as_void = current_;
    *as_size_t = (size - sizeof(size_t)); // store out size.
    as_char += sizeof(size_t);

    void* userPtr = as_char; // save the pointer we return.
    current_ += size;        // set current position.

#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
    // stats baby !
    statistics_.physicalMemoryUsed_ = safe_static_cast<size_t>(current_ - start_);

    statistics_.wasteAlignmentMax_ = Max(statistics_.wasteAlignment_, statistics_.wasteAlignmentMax_);
    statistics_.allocationCountMax_ = Max(statistics_.allocationCount_, statistics_.allocationCountMax_);
    statistics_.internalOverheadMax_ = Max(statistics_.internalOverhead_, statistics_.internalOverheadMax_);
    statistics_.physicalMemoryUsedMax_ = Max(statistics_.physicalMemoryUsed_, statistics_.physicalMemoryUsedMax_);
#endif

    return userPtr;
}

MemoryAllocatorStatistics LinearAllocator::getStatistics(void) const
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