#include "EngineCommon.h"

#include "GrowingPoolAllocator.h"

#include "Util/BitUtil.h"
#include "memory/VirtualMem.h"

X_NAMESPACE_BEGIN(core)

namespace
{
    size_t CalculateElementSize(size_t maxElementSize, size_t maxAlignment)
    {
        return bitUtil::RoundUpToMultiple(maxElementSize, maxAlignment);
    }

    size_t CalculateWasteAtFront(void* start, size_t maxAlignment, size_t offset)
    {
        char* pAdd = pointerUtil::AlignTop(reinterpret_cast<char*>(start) + offset, maxAlignment);

        return safe_static_cast<size_t>(union_cast<uintptr_t>(pAdd - offset)
                                        - union_cast<uintptr_t>(start));
    }

} // namespace

GrowingPoolAllocator::GrowingPoolAllocator(size_t maxSizeInBytes, size_t growSize,
    size_t chunkHeaderSize, size_t maxElementSize, size_t maxAlignment, size_t offset)
{
    virtualStart_ = static_cast<char*>(VirtualMem::ReserveAddressSpace(maxSizeInBytes));
    virtualEnd_ = &virtualStart_[maxSizeInBytes];

    physicalCurrent_ = virtualStart_;
    growSize_ = growSize;
    chunkHeaderSize_ = chunkHeaderSize;
    maxSize_ = maxElementSize;
    maxAlignment_ = maxAlignment;

#if X_ENABLE_POOL_ALLOCATOR_CHECK
    offset_ = offset;
#else
    X_UNUSED(offset);
#endif

    X_ASSERT(bitUtil::IsPowerOfTwo(growSize), "Pool Grow size must be a power-of-two.")
    (growSize);

    if (maxSizeInBytes % growSize) {
        X_ASSERT(false, "Maximum amount of virtual address space to reserve must be a multiple of the grow size.")
        (growSize);
    }
    if (growSize % VirtualMem::GetPageSize()) {
        X_ASSERT(false, "Pool grow size must be a multiple of virtual page size.")
        (growSize);
    }

#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
    zero_object(statistics_);

    statistics_.virtualMemoryReserved_ = maxSizeInBytes;

    elementSize_ = CalculateElementSize(maxElementSize, maxAlignment);
    wastePerElement_ = maxElementSize - elementSize_;

    statistics_.type_ = "GrowPoolAllocator";
#endif
}

/// Frees all physical memory owned by the allocator.
GrowingPoolAllocator::~GrowingPoolAllocator(void)
{
    VirtualMem::ReleaseAddressSpace(virtualStart_);
}

void* GrowingPoolAllocator::allocate(size_t size, size_t alignment, size_t offset,
    const void* chunkHeaderData, size_t chunkHeaderSize)
{
    if (size > maxSize_) {
        X_ASSERT(false, "Pool allocator can't satsify a request bigger than max size only equal or less")
        (size, maxSize_);
    }

    if (alignment > maxAlignment_) {
        X_ASSERT(false, "Pool allocaotr alignment must be equal or less")
        (alignment, maxAlignment_);
    }

#if X_ENABLE_POOL_ALLOCATOR_CHECK
    if (offset != offset_) {
        X_ASSERT(false, "A pool allocator can only allocate instances with the same offset.")
        (offset, offset_);
    }
#endif

    void* pMem = freelist_.Obtain();
    if (pMem) {
        // info for when we either grow or not.
#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS

        statistics_.allocationCount_++;
        statistics_.physicalMemoryUsed_ += elementSize_;
        statistics_.wasteAlignment_ += wastePerElement_;

        statistics_.allocationCountMax_ = Max(statistics_.allocationCountMax_, statistics_.allocationCount_);
        statistics_.physicalMemoryUsedMax_ = Max(statistics_.physicalMemoryUsedMax_, statistics_.physicalMemoryUsed_);
        statistics_.wasteAlignmentMax_ = Max(statistics_.wasteAlignmentMax_, statistics_.wasteAlignment_);
#endif
        return pMem;
    }

    // we need to grow baby.
    size_t neededPhysicalSize = bitUtil::RoundUpToMultiple<size_t>(offset + maxSize_, growSize_);

    // can this even happen?
    if (&physicalCurrent_[neededPhysicalSize] > virtualEnd_) {
        return nullptr;
    }

    void* pMemoryRegionStart = VirtualMem::AllocatePhysicalMemory(physicalCurrent_, neededPhysicalSize);

    physicalCurrent_ += neededPhysicalSize;

    void* pChunkHeaderStart = physicalCurrent_ - chunkHeaderSize_;

    freelist_ = Freelist(pMemoryRegionStart, pChunkHeaderStart, maxSize_, maxAlignment_, offset);

    // ok now try.
    pMem = freelist_.Obtain();

    if (chunkHeaderData) {
        memcpy(pChunkHeaderStart, chunkHeaderData, chunkHeaderSize);
    }

    // update all dat info.
#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
    size_t wasteAtFront = CalculateWasteAtFront(pMemoryRegionStart, maxAlignment_, offset);
    size_t memoryRegionSize = safe_static_cast<uint32_t>(reinterpret_cast<char*>(pChunkHeaderStart) - reinterpret_cast<char*>(pMemoryRegionStart));
    size_t elementCount = (memoryRegionSize - wasteAtFront) / elementSize_;

    statistics_.physicalMemoryAllocated_ = safe_static_cast<uint32_t>(
        physicalCurrent_ - virtualStart_);

    statistics_.physicalMemoryUsed_ += chunkHeaderSize_ + wasteAtFront;
    statistics_.wasteAlignment_ += wasteAtFront;
    statistics_.wasteUnused_ = chunkHeaderSize_ + memoryRegionSize - wasteAtFront - elementSize_ * elementCount;

    // 1,2,3 what is the max? I hope i don't have to pay tax.
    statistics_.physicalMemoryAllocatedMax_ = Max(statistics_.physicalMemoryAllocatedMax_, statistics_.physicalMemoryAllocated_);
    statistics_.physicalMemoryUsedMax_ = Max(statistics_.physicalMemoryUsedMax_, statistics_.physicalMemoryUsed_);
    statistics_.wasteAlignmentMax_ = Max(statistics_.wasteAlignmentMax_, statistics_.wasteAlignment_);
    statistics_.wasteUnusedMax_ = Max(statistics_.wasteUnusedMax_, statistics_.wasteUnused_);

    // info for when we either grow or not.
    statistics_.allocationCount_++;
    statistics_.physicalMemoryUsed_ += elementSize_;
    statistics_.wasteAlignment_ += wastePerElement_;

    statistics_.allocationCountMax_ = Max(statistics_.allocationCountMax_, statistics_.allocationCount_);
    statistics_.physicalMemoryUsedMax_ = Max(statistics_.physicalMemoryUsedMax_, statistics_.physicalMemoryUsed_);
    statistics_.wasteAlignmentMax_ = Max(statistics_.wasteAlignmentMax_, statistics_.wasteAlignment_);
#endif

    return pMem;
}

MemoryAllocatorStatistics GrowingPoolAllocator::getStatistics(void) const
{
#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS
    return statistics_;
#else
    static MemoryAllocatorStatistics stats;
    core::zero_object(stats);
    return stats;
#endif
}

X_NAMESPACE_END