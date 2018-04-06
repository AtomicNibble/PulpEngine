#include "stdafx.h"

#include <Memory/AllocationPolicies/LinearAllocator.h>
#include <Memory\VirtualMem.h>

X_USING_NAMESPACE;

using namespace core;

TEST(LinearAlloc, Stack)
{
    X_ALIGNED_SYMBOL(char memory[4096], 64) = {};
    X_ASSERT_ALIGNMENT(memory, 64, 0);

    core::LinearAllocator allocator(memory, memory + 4096);

    {
        void* alloc = allocator.allocate(4, 32, 4);
        X_ASSERT_NOT_NULL(alloc);
        X_ASSERT_ALIGNMENT(alloc, 32, 4);
        memset(alloc, 0xAB, 4);

        // with an offset of 4, the allocation has to leave a gap of (32-4 = 28) bytes
        EXPECT_EQ(4, allocator.getSize(alloc));
        EXPECT_EQ(memory + 28, alloc);
        allocator.free(alloc);
    }

    {
        void* alloc = allocator.allocate(16, 16, 0);
        X_ASSERT_NOT_NULL(alloc);
        X_ASSERT_ALIGNMENT(alloc, 16, 0);
        memset(alloc, 0xAB, 16);

        // the first allocation (4 bytes) must end at a 16-byte boundary, and with the internally
        // added offset of sizeof(size_t), we end up at the next 16-byte boundary
        EXPECT_EQ(16, allocator.getSize(alloc));
        EXPECT_EQ(memory + 48, alloc);
        allocator.free(alloc);
    }

    allocator.reset();

    {
        // we've got space for exactly 4096 bytes, try to allocate as much
        void* alloc = allocator.allocate(4096 - sizeof(size_t), 4, 0);
        X_ASSERT_NOT_NULL(alloc);
        X_ASSERT_ALIGNMENT(alloc, 4, 0);
        memset(alloc, 0xAB, 4096 - sizeof(size_t));

        EXPECT_EQ(4096 - sizeof(size_t), allocator.getSize(alloc));
        allocator.free(alloc);

        core::debugging::EnableBreakpoints(false);

        // no more space left
        g_AssetChecker.ExpectAssertion(true);

        void* failedAlloc = allocator.allocate(4, 4, 0);
        EXPECT_TRUE(failedAlloc == nullptr);
        allocator.free(failedAlloc);

        EXPECT_TRUE(g_AssetChecker.HadCorrectAssertions());

        g_AssetChecker.ExpectAssertion(false);

        core::debugging::EnableBreakpoints(true);

        allocator.reset();
    }
}

TEST(LinearAlloc, Heap)
{
    core::HeapArea heapArea(core::VirtualMem::GetPageSize());
    X_ASSERT_ALIGNMENT(heapArea.start(), 32, 0);
    char* memory = static_cast<char*>(heapArea.start());

    core::LinearAllocator allocator(memory, memory + core::VirtualMem::GetPageSize());

    {
        void* alloc = allocator.allocate(4, 32, 4);
        X_ASSERT_NOT_NULL(alloc);
        X_ASSERT_ALIGNMENT(alloc, 32, 4);
        memset(alloc, 0xAB, 4);

        // with an offset of 4, the allocation has to leave a gap of (32-4 = 28) bytes
        EXPECT_EQ(4, allocator.getSize(alloc));
        EXPECT_EQ(memory + 28, alloc);
        allocator.free(alloc);
    }

    {
        void* alloc = allocator.allocate(16, 16, 0);
        X_ASSERT_NOT_NULL(alloc);
        X_ASSERT_ALIGNMENT(alloc, 16, 0);
        memset(alloc, 0xAB, 16);

        // the first allocation (4 bytes) must end at a 16-byte boundary, and with the internally
        // added offset of sizeof(size_t), we end up at the next 16-byte boundary
        EXPECT_EQ(16, allocator.getSize(alloc));
        EXPECT_EQ(memory + 48, alloc);
        allocator.free(alloc);
    }

    allocator.reset();

    {
        // we've got space for exactly 4096 bytes, try to allocate as much
        void* alloc = allocator.allocate(4096 - sizeof(size_t), 4, 0);
        X_ASSERT_NOT_NULL(alloc);
        X_ASSERT_ALIGNMENT(alloc, 4, 0);
        memset(alloc, 0xAB, 4096 - sizeof(size_t));

        EXPECT_EQ(4096 - sizeof(size_t), allocator.getSize(alloc));
        allocator.free(alloc);

        core::debugging::EnableBreakpoints(false);

        g_AssetChecker.ExpectAssertion(true);

        // no more space left
        void* failedAlloc = allocator.allocate(4, 4, 0);
        EXPECT_TRUE(failedAlloc == nullptr);

        allocator.free(failedAlloc);

        EXPECT_TRUE(g_AssetChecker.HadCorrectAssertions());
        g_AssetChecker.ExpectAssertion(false);

        core::debugging::EnableBreakpoints(true);

        allocator.reset();
    }
}

#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS

TEST(LinearAlloc, Stats)
{
    X_ALIGNED_SYMBOL(char memory[4096], 32) = {};
    X_ASSERT_ALIGNMENT(memory, 32, 0);

    core::LinearAllocator allocator(memory, memory + 4096);

    {
        const core::MemoryAllocatorStatistics& stats = allocator.getStatistics();
        EXPECT_EQ(stats.allocationCount_, 0);
        EXPECT_EQ(stats.allocationCountMax_, 0);
        EXPECT_EQ(stats.virtualMemoryReserved_, 4096);
        EXPECT_EQ(stats.physicalMemoryAllocated_, 4096);
        EXPECT_EQ(stats.physicalMemoryAllocatedMax_, 4096);
        EXPECT_EQ(stats.physicalMemoryUsed_, 0);
        EXPECT_EQ(stats.physicalMemoryUsedMax_, 0);
        EXPECT_EQ(stats.wasteAlignment_, 0);
        EXPECT_EQ(stats.wasteAlignmentMax_, 0);
        EXPECT_EQ(stats.wasteUnused_, 0);
        EXPECT_EQ(stats.wasteUnusedMax_, 0);
        EXPECT_EQ(stats.internalOverhead_, 0);
        EXPECT_EQ(stats.internalOverheadMax_, 0);
    }

    {
        void* alloc = allocator.allocate(100, 32, 0);
        X_ASSERT_NOT_NULL(alloc);
        X_ASSERT_ALIGNMENT(alloc, 32, 0);
        memset(alloc, 0xAB, 100);

        {
            const core::MemoryAllocatorStatistics& stats = allocator.getStatistics();
            EXPECT_EQ(stats.allocationCount_, 1);
            EXPECT_EQ(stats.allocationCountMax_, 1);
            EXPECT_EQ(stats.virtualMemoryReserved_, 4096);
            EXPECT_EQ(stats.physicalMemoryAllocated_, 4096);
            EXPECT_EQ(stats.physicalMemoryAllocatedMax_, 4096);
#if X_64
            EXPECT_EQ(stats.physicalMemoryUsed_, 124 + sizeof(size_t));
            EXPECT_EQ(stats.physicalMemoryUsedMax_, 124 + sizeof(size_t));
            EXPECT_EQ(stats.wasteAlignment_, 24);
            EXPECT_EQ(stats.wasteAlignmentMax_, 24);
#else
            EXPECT_EQ(stats.physicalMemoryUsed_, 128 + sizeof(size_t));
            EXPECT_EQ(stats.physicalMemoryUsedMax_, 128 + sizeof(size_t));
            EXPECT_EQ(stats.wasteAlignment_, 28);
            EXPECT_EQ(stats.wasteAlignmentMax_, 28);
#endif
            EXPECT_EQ(stats.wasteUnused_, 0);
            EXPECT_EQ(stats.wasteUnusedMax_, 0);
            EXPECT_EQ(stats.internalOverhead_, sizeof(size_t));
            EXPECT_EQ(stats.internalOverheadMax_, sizeof(size_t));
        }

        allocator.free(alloc);

        {
            const core::MemoryAllocatorStatistics& stats = allocator.getStatistics();
            EXPECT_EQ(stats.allocationCount_, 1);
            EXPECT_EQ(stats.allocationCountMax_, 1);
            EXPECT_EQ(stats.virtualMemoryReserved_, 4096);
            EXPECT_EQ(stats.physicalMemoryAllocated_, 4096);
            EXPECT_EQ(stats.physicalMemoryAllocatedMax_, 4096);

#if X_64
            EXPECT_EQ(stats.physicalMemoryUsed_, 124 + sizeof(size_t));
            EXPECT_EQ(stats.physicalMemoryUsedMax_, 124 + sizeof(size_t));
            EXPECT_EQ(stats.wasteAlignment_, 24);
            EXPECT_EQ(stats.wasteAlignmentMax_, 24);
#else
            EXPECT_EQ(stats.physicalMemoryUsed_, 128 + sizeof(size_t));
            EXPECT_EQ(stats.physicalMemoryUsedMax_, 128 + sizeof(size_t));
            EXPECT_EQ(stats.wasteAlignment_, 28);
            EXPECT_EQ(stats.wasteAlignmentMax_, 28);
#endif

            EXPECT_EQ(stats.wasteUnused_, 0);
            EXPECT_EQ(stats.wasteUnusedMax_, 0);
            EXPECT_EQ(stats.internalOverhead_, sizeof(size_t));
            EXPECT_EQ(stats.internalOverheadMax_, sizeof(size_t));
        }
    }

    {
        void* alloc = allocator.allocate(124, 32, 0);
        X_ASSERT_NOT_NULL(alloc);
        X_ASSERT_ALIGNMENT(alloc, 32, 0);
        memset(alloc, 0xAB, 124);

        {
            const core::MemoryAllocatorStatistics& stats = allocator.getStatistics();
            EXPECT_EQ(stats.allocationCount_, 2);
            EXPECT_EQ(stats.allocationCountMax_, 2);
            EXPECT_EQ(stats.virtualMemoryReserved_, 4096);
            EXPECT_EQ(stats.physicalMemoryAllocated_, 4096);
            EXPECT_EQ(stats.physicalMemoryAllocatedMax_, 4096);
#if X_64
            EXPECT_EQ(stats.physicalMemoryUsed_, 124 + sizeof(size_t) + 124 + sizeof(size_t) + 20);
            EXPECT_EQ(stats.physicalMemoryUsedMax_, 124 + sizeof(size_t) + 124 + sizeof(size_t) + 20);
            EXPECT_EQ(stats.wasteAlignment_, 44);
            EXPECT_EQ(stats.wasteAlignmentMax_, 44);
#else
            EXPECT_EQ(stats.physicalMemoryUsed_, 128 + sizeof(size_t) + 128 + sizeof(size_t) + 20);
            EXPECT_EQ(stats.physicalMemoryUsedMax_, 128 + sizeof(size_t) + 128 + sizeof(size_t) + 20);
            EXPECT_EQ(stats.wasteAlignment_, 52);
            EXPECT_EQ(stats.wasteAlignmentMax_, 52);
#endif
            EXPECT_EQ(stats.wasteUnused_, 0);
            EXPECT_EQ(stats.wasteUnusedMax_, 0);
            EXPECT_EQ(stats.internalOverhead_, sizeof(size_t) * 2);
            EXPECT_EQ(stats.internalOverheadMax_, sizeof(size_t) * 2);
        }

        allocator.free(alloc);

        {
            const core::MemoryAllocatorStatistics& stats = allocator.getStatistics();
            EXPECT_EQ(stats.allocationCount_, 2);
            EXPECT_EQ(stats.allocationCountMax_, 2);
            EXPECT_EQ(stats.virtualMemoryReserved_, 4096);
            EXPECT_EQ(stats.physicalMemoryAllocated_, 4096);
            EXPECT_EQ(stats.physicalMemoryAllocatedMax_, 4096);
#if X_64
            EXPECT_EQ(stats.physicalMemoryUsed_, 124 + sizeof(size_t) + 124 + sizeof(size_t) + 20);
            EXPECT_EQ(stats.physicalMemoryUsedMax_, 124 + sizeof(size_t) + 124 + sizeof(size_t) + 20);
            EXPECT_EQ(stats.wasteAlignment_, 44);
            EXPECT_EQ(stats.wasteAlignmentMax_, 44);
#else
            EXPECT_EQ(stats.physicalMemoryUsed_, 128 + sizeof(size_t) + 128 + sizeof(size_t) + 20);
            EXPECT_EQ(stats.physicalMemoryUsedMax_, 128 + sizeof(size_t) + 128 + sizeof(size_t) + 20);
            EXPECT_EQ(stats.wasteAlignment_, 52);
            EXPECT_EQ(stats.wasteAlignmentMax_, 52);
#endif

            EXPECT_EQ(stats.wasteUnused_, 0);
            EXPECT_EQ(stats.wasteUnusedMax_, 0);
            EXPECT_EQ(stats.internalOverhead_, sizeof(size_t) * 2);
            EXPECT_EQ(stats.internalOverheadMax_, sizeof(size_t) * 2);
        }

        allocator.reset();

        {
            const core::MemoryAllocatorStatistics& stats = allocator.getStatistics();
            EXPECT_EQ(stats.allocationCount_, 0);
            EXPECT_EQ(stats.allocationCountMax_, 2);
            EXPECT_EQ(stats.virtualMemoryReserved_, 4096);
            EXPECT_EQ(stats.physicalMemoryAllocated_, 4096);
            EXPECT_EQ(stats.physicalMemoryAllocatedMax_, 4096);
            EXPECT_EQ(stats.physicalMemoryUsed_, 0);
#if X_64
            EXPECT_EQ(stats.physicalMemoryUsedMax_, 124 + sizeof(size_t) + 124 + sizeof(size_t) + 20);
            EXPECT_EQ(stats.wasteAlignment_, 0);
            EXPECT_EQ(stats.wasteAlignmentMax_, 44);
#else
            EXPECT_EQ(stats.physicalMemoryUsedMax_, 128 + sizeof(size_t) + 128 + sizeof(size_t) + 20);
            EXPECT_EQ(stats.wasteAlignment_, 0);
            EXPECT_EQ(stats.wasteAlignmentMax_, 52);
#endif
            EXPECT_EQ(stats.wasteUnused_, 0);
            EXPECT_EQ(stats.wasteUnusedMax_, 0);
            EXPECT_EQ(stats.internalOverhead_, 0);
            EXPECT_EQ(stats.internalOverheadMax_, sizeof(size_t) * 2);
        }
    }
}

#endif // X_ENABLE_MEMORY_ALLOCATOR_STATISTICS