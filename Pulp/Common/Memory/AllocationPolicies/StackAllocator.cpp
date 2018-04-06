#include "EngineCommon.h"

#include "StackAllocator.h"

#include "Util\PointerUtil.h"

X_NAMESPACE_BEGIN(core)

StackAllocator::StackAllocator(void* start, void* end) :
    start_((char*)start),
    end_((char*)end),
    current_((char*)start)
{
#if X_ENABLE_STACK_ALLOCATOR_CHECK
    allocationID_ = 0;
#endif

#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
    zero_this(&statistics_);

    size_t Len = safe_static_cast<size_t, uintptr_t>((uintptr_t)end - (uintptr_t)start);

    statistics_.type_ = "Stack";
    statistics_.virtualMemoryReserved_ = Len;
    statistics_.physicalMemoryAllocated_ = Len;
    statistics_.physicalMemoryAllocatedMax_ = Len;
#endif
}

void* StackAllocator::allocate(size_t size, size_t alignment, size_t align_offset)
{
    size_t AllocationSize = size;

    size += sizeof(BlockHeader);         // add room for our book keeping.
    align_offset += sizeof(BlockHeader); // offset includes base position.

    // get current relative offset
    const uint32_t allocationOffset = safe_static_cast<uint32_t>(current_ - start_);
#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
    const char* old_current = current_;
#endif // !X_ENABLE_MEMORY_ALLOCATOR_STATISTICS

    // align it at the offset position, then go back the offset to bring us to start.
    // of return data not actual start as alignment can waste some memory.
    current_ = pointerUtil::AlignTop(current_ + align_offset, alignment) - align_offset;

    // Do we even have room slut?
    if ((current_ + size) > end_) {
        X_ASSERT((current_ + size) <= end_, "Stack overflow!, a stack allocator can't satisfy the request.")();
        return nullptr;
    }

#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
    // stats baby !
    statistics_.allocationCount_++;
    statistics_.internalOverhead_ += sizeof(BlockHeader);
    statistics_.physicalMemoryUsed_ += size + (current_ - old_current);
    statistics_.wasteAlignment_ += safe_static_cast<size_t>(((uintptr_t)current_ - (uintptr_t)start_) - allocationOffset);

    statistics_.wasteAlignmentMax_ = Max(statistics_.wasteAlignment_, statistics_.wasteAlignmentMax_);
    statistics_.allocationCountMax_ = Max(statistics_.allocationCount_, statistics_.allocationCountMax_);
    statistics_.internalOverheadMax_ = Max(statistics_.internalOverhead_, statistics_.internalOverheadMax_);
    statistics_.physicalMemoryUsedMax_ = Max(statistics_.physicalMemoryUsed_, statistics_.physicalMemoryUsedMax_);

#endif

    union
    {
        void* as_void;
        char* as_char;
        BlockHeader* as_header;
    };

    as_char = current_;
#if X_ENABLE_STACK_ALLOCATOR_CHECK
    as_header->AllocationID_ = allocationID_++;
#endif
    as_header->allocationOffset_ = allocationOffset;
    as_header->AllocationSize_ = AllocationSize;
    as_char += sizeof(BlockHeader);

    void* userPtr = as_void;
    current_ += size;
    return userPtr;
}

void StackAllocator::free(void* ptr)
{
    X_ASSERT_NOT_NULL(ptr);

    union
    {
        void* as_void;
        char* as_char;
        BlockHeader* as_header;
    };

    as_void = ptr;
    as_char -= sizeof(BlockHeader);

#if X_ENABLE_STACK_ALLOCATOR_CHECK
    if (as_header->AllocationID_ != (allocationID_ - 1)) {
        uint32_t AllocationID = as_header->AllocationID_;
        X_ASSERT(false, "Cannot free memory from stack(LIFO). invalid order.")(allocationID_, AllocationID);
    }

    allocationID_--;
#endif

    current_ = start_ + as_header->allocationOffset_;

#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
    statistics_.allocationCount_--;
    statistics_.physicalMemoryUsed_ = as_header->allocationOffset_;
    statistics_.internalOverhead_ -= sizeof(BlockHeader);
    statistics_.wasteAlignment_ -= safe_static_cast<uint32_t>(as_char - current_);
#endif
}

void StackAllocator::free(void* ptr, size_t size)
{
    X_UNUSED(size);
    free(ptr);
}

MemoryAllocatorStatistics StackAllocator::getStatistics(void) const
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