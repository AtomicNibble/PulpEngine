#include "EngineCommon.h"

#include "MallocFreeAllocator.h"

#include "Util\PointerUtil.h"

X_NAMESPACE_BEGIN(core)

namespace
{
    //	static const size_t SIZE_OF_HEADER = sizeof(MallocFreeAllocator::BlockHeader);

#if X_64
    static_assert(MallocFreeAllocator::SIZE_OF_HEADER == 24, "Allocation header has wrong size.");
#else // X_64
    static_assert(MallocFreeAllocator::SIZE_OF_HEADER == 12, "Allocation header has wrong size.");
#endif // X_64

} // namespace

MallocFreeAllocator::MallocFreeAllocator(void)
{
#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
    zero_object(statistics_);

    statistics_.type_ = "MallocFree";
#endif // X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
}

void* MallocFreeAllocator::allocate(size_t Origsize, size_t alignment, size_t offset)
{
    X_ASSERT(alignment > 0, "Alignment must be greater than zero")(alignment);

    size_t size = Origsize + alignment + (SIZE_OF_HEADER);

    tt_uint64 matchId;
    X_UNUSED(matchId);
    ttEnterEx(gEnv->ctx, matchId, 1000, "(Memory) Alloc");

    void* pMem = malloc(size);
    ttAlloc(gEnv->ctx, pMem, size, "Malloc");

    ttLeaveEx(gEnv->ctx, matchId);

    if (pMem) {
        union
        {
            BlockHeader* as_header;
            void* as_void;
            char* as_byte;
        };

        // get the aligned pointer.
        as_void = pointerUtil::AlignBottom<char>(reinterpret_cast<char*>(pMem) + offset + alignment + (SIZE_OF_HEADER - 1), alignment);

        as_byte -= offset; // take off any offset.

        // we have taken off offset so now header is just at -1 index.
        as_header[-1].originalAllocation_ = pMem;
        as_header[-1].AllocationSize_ = size;
        as_header[-1].originalSize_ = Origsize;

#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
        statistics_.totalAllocations_++;
        statistics_.totalAllocationSize_ += size;
        statistics_.allocationCount_++;
        statistics_.virtualMemoryReserved_ += size;
        statistics_.physicalMemoryAllocated_ += size;
        statistics_.physicalMemoryUsed_ += size;
        statistics_.internalOverhead_ += SIZE_OF_HEADER;
        statistics_.wasteAlignment_ += safe_static_cast<size_t>((uintptr_t)as_byte - (uintptr_t)pMem) - SIZE_OF_HEADER;

        statistics_.allocationCountMax_ = Max(statistics_.allocationCount_, statistics_.allocationCountMax_);
        statistics_.physicalMemoryUsedMax_ = Max(statistics_.physicalMemoryUsed_, statistics_.physicalMemoryUsedMax_);
        statistics_.physicalMemoryAllocatedMax_ = Max(statistics_.physicalMemoryAllocated_, statistics_.physicalMemoryAllocatedMax_);
        statistics_.wasteAlignmentMax_ = Max(statistics_.wasteAlignment_, statistics_.wasteAlignmentMax_);
        statistics_.internalOverheadMax_ = Max(statistics_.internalOverhead_, statistics_.internalOverheadMax_);
#endif // X_ENABLE_MEMORY_ALLOCATOR_STATISTICS

        return as_byte; // we return the mem - offset.
    }

    return nullptr;
}

void MallocFreeAllocator::free(void* ptr)
{
    X_ASSERT_NOT_NULL(ptr);

    union
    {
        BlockHeader* as_header;
        void* as_void;
        char* as_byte;
    };

    as_void = ptr;
    as_byte -= SIZE_OF_HEADER;

#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
    size_t AlignmentWaste = safe_static_cast<size_t>(reinterpret_cast<uintptr_t>(ptr) - reinterpret_cast<uintptr_t>(as_header->originalAllocation_)) - SIZE_OF_HEADER;

    size_t Size = as_header->AllocationSize_;

    statistics_.allocationCount_--;
    statistics_.virtualMemoryReserved_ -= Size;
    statistics_.physicalMemoryAllocated_ -= Size;
    statistics_.physicalMemoryUsed_ -= Size;
    statistics_.internalOverhead_ -= SIZE_OF_HEADER;
    statistics_.wasteAlignment_ -= AlignmentWaste;
#endif // X_ENABLE_MEMORY_ALLOCATOR_STATISTICS

    // must read address before we free, or make a copy.
    ttFree(gEnv->ctx, as_header->originalAllocation_);

    ::free(as_header->originalAllocation_);
}

void MallocFreeAllocator::free(void* ptr, size_t size)
{
    X_UNUSED(size);
    free(ptr);
}

MemoryAllocatorStatistics MallocFreeAllocator::getStatistics(void) const
{
#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
    return statistics_;
#else // X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
    static MemoryAllocatorStatistics stats;
    core::zero_object(stats);
    return stats;
#endif // X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
}

X_NAMESPACE_END
