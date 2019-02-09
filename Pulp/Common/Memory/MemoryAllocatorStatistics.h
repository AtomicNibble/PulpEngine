#pragma once

#ifndef X_MEMORYALLOCATORSTATISTICS_H_
#define X_MEMORYALLOCATORSTATISTICS_H_

#include "String\HumanSize.h"

X_NAMESPACE_BEGIN(core)

struct MemoryAllocatorStatistics
{
    typedef core::StackString512 Str;

    void Clear(void)
    {
        // don't clear the char pointer.
        memset((&type_) + 1, 0, sizeof(MemoryAllocatorStatistics) - sizeof(type_));
    }

    const char* toString(Str& str, bool incMax = false) const
    {
        core::HumanSize::Str temp;

        str.clear();
        str.appendFmt("Requests: ^6%" PRIuS "^~\n", totalAllocations_);
        str.appendFmt("Num: ^6%" PRIuS "^~\n", allocationCount_);
        str.appendFmt("Num(Max): ^6%" PRIuS "^~\n", allocationCountMax_);
        str.appendFmt("Physical: ^6%s^~\n", core::HumanSize::toString(temp, physicalMemoryAllocated_));
        str.appendFmt("Physical(Used): ^6%s^~\n", core::HumanSize::toString(temp, physicalMemoryUsed_));
        str.appendFmt("Virtual(Res): ^6%s^~\n", core::HumanSize::toString(temp, virtualMemoryReserved_));
        str.appendFmt("WasteAlign: ^6%s^~\n", core::HumanSize::toString(temp, wasteAlignment_));
        str.appendFmt("WasteUnused: ^6%s^~\n", core::HumanSize::toString(temp, wasteUnused_));
        str.appendFmt("Overhead: ^6%s^~\n", core::HumanSize::toString(temp, internalOverhead_));

        if (incMax) {
            str.appendFmt("Physical(Max): ^6%s^~\n", core::HumanSize::toString(temp, physicalMemoryAllocatedMax_));
            str.appendFmt("Physical(used-Max): ^6%s^~\n", core::HumanSize::toString(temp, physicalMemoryUsedMax_));
            str.appendFmt("WasteAlign(Max): ^6%s^~\n", core::HumanSize::toString(temp, wasteAlignmentMax_));
            str.appendFmt("WasteUnused(Max): ^6%s^~\n", core::HumanSize::toString(temp, wasteUnusedMax_));
            str.appendFmt("Overhead(Max): ^6%s", core::HumanSize::toString(temp, internalOverheadMax_));
        }

        return str.c_str();
    }

    MemoryAllocatorStatistics& operator+=(const MemoryAllocatorStatistics& oth)
    {
        totalAllocations_ += oth.totalAllocations_;
        totalAllocationSize_ += oth.totalAllocationSize_;
        allocationCount_ += oth.allocationCount_;
        allocationCountMax_ += oth.allocationCountMax_;

        virtualMemoryReserved_ += oth.virtualMemoryReserved_;

        physicalMemoryAllocated_ += oth.physicalMemoryAllocated_;
        physicalMemoryAllocatedMax_ += oth.physicalMemoryAllocatedMax_;
        physicalMemoryUsed_ += oth.physicalMemoryUsed_;
        physicalMemoryUsedMax_ += oth.physicalMemoryUsedMax_;

        wasteAlignment_ += oth.wasteAlignment_;
        wasteAlignmentMax_ += oth.wasteAlignmentMax_;
        wasteUnused_ += oth.wasteUnused_;
        wasteUnusedMax_ += oth.wasteUnusedMax_;
        internalOverhead_ += oth.internalOverhead_;
        internalOverheadMax_ += oth.internalOverheadMax_;
        return *this;
    }

    // general
    const char* type_; ///< A human-readable string describing the type of allocator.

    // allocations
    size_t totalAllocations_;        ///< The total number of allocations.
    size_t totalAllocationSize_;     ///< The total size of all allocations.
    size_t allocationCount_;    ///< The current number of allocations.
    size_t allocationCountMax_; ///< The maximum number of allocations in flight at any given point in time.

    // virtual memory
    size_t virtualMemoryReserved_; ///< The amount of virtual memory reserved.

    // physical memory
    size_t physicalMemoryAllocated_;    ///< The amount of physical memory currently allocated.
    size_t physicalMemoryAllocatedMax_; ///< The maximum amount of physical memory that was allocated.
    size_t physicalMemoryUsed_;         ///< The amount of physical memory actually in use.
    size_t physicalMemoryUsedMax_;      ///< The maximum amount of physical memory that was in use.

    // overhead
    size_t wasteAlignment_;      ///< The amount of wasted memory due to alignment.
    size_t wasteAlignmentMax_;   ///< The maximum amount of memory that was wasted due to alignment.
    size_t wasteUnused_;         ///< The amount of memory that cannot be used for satisfying allocation requests.
    size_t wasteUnusedMax_;      ///< The maximum amount of memory that could not be used for satisfying allocation requests.
    size_t internalOverhead_;    ///< The internal overhead caused by book-keeping information.
    size_t internalOverheadMax_; ///< The maximum amount of internal overhead that was caused by book-keeping information.
};

X_NAMESPACE_END

#endif // X_MEMORYALLOCATORSTATISTICS_H_
