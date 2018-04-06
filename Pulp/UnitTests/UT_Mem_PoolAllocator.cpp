#include "stdafx.h"

#include <Memory\AllocationPolicies\GrowingPoolAllocator.h>
#include <Memory\AllocationPolicies\PoolAllocator.h>
#include <Memory\VirtualMem.h>

X_USING_NAMESPACE;

using namespace core;

TEST(PoolAlloc, Stack)
{
    X_ALIGNED_SYMBOL(char memory[4096], 64) = {};
    X_ASSERT_ALIGNMENT(memory, 64, 0);

    for (unsigned int N = 8; N <= 64; N <<= 1) {
        // with an element size of N and a maximum alignment of N and no offset, there should be space for 4096/N allocations
        const unsigned int allocationCount = 4096 / N;

        core::PoolAllocator allocator(memory, memory + 4096, N, N, 0);

        for (unsigned int i = 0; i < allocationCount; ++i) {
            void* alloc = allocator.allocate(N, N, 0);
            X_ASSERT_NOT_NULL(alloc);
            X_ASSERT_ALIGNMENT(alloc, N, 0);

            ASSERT_TRUE(alloc != NULL);
            ASSERT_TRUE(core::internal::IsAligned(alloc, N, 0));
            memset(alloc, 0xAB, N);

            EXPECT_EQ(allocator.getSize(alloc), N);
        }

        core::debugging::EnableBreakpoints(false);

        g_AssetChecker.ExpectAssertion(true);

        // there should be no space left
        void* failedAlloc = allocator.allocate(4, 4, 0);
        EXPECT_TRUE(failedAlloc == nullptr);
        EXPECT_TRUE(g_AssetChecker.HadCorrectAssertions());

        g_AssetChecker.ExpectAssertion(false);

        core::debugging::EnableBreakpoints(true);
    }

    {
        core::PoolAllocator allocator(memory, memory + 4096, 32, 32, 0);

        // making allocations smaller than the element size should be allowed
        void* alloc0 = allocator.allocate(4, 32, 0);
        X_ASSERT_NOT_NULL(alloc0);
        X_ASSERT_ALIGNMENT(alloc0, 32, 0);
        EXPECT_EQ(allocator.getSize(alloc0), 32);

        // making allocations with a smaller alignment should be allowed as well
        void* alloc1 = allocator.allocate(32, 4, 0);
        X_ASSERT_NOT_NULL(alloc1);
        X_ASSERT_ALIGNMENT(alloc1, 4, 0);
        EXPECT_EQ(allocator.getSize(alloc1), 32);

        void* alloc2 = allocator.allocate(4, 4, 0);
        X_ASSERT_NOT_NULL(alloc2);
        X_ASSERT_ALIGNMENT(alloc2, 4, 0);
        EXPECT_EQ(allocator.getSize(alloc2), 32);

#if X_ENABLE_POOL_ALLOCATOR_CHECK
        // larger element sizes or alignments are disallowed
/*		X_UT_EXPECT_ASSERTION();
		void* alloc3 = allocator.allocate(40, 4, 0);
		X_UT_TEST_ASSERTION();

		X_UT_EXPECT_ASSERTION();
		void* alloc4 = allocator.allocate(16, 64, 0);
		X_UT_TEST_ASSERTION();

		allocator.free(alloc4);
		allocator.free(alloc3);
		*/
#endif

        allocator.free(alloc2);
        allocator.free(alloc1);
        allocator.free(alloc0);
    }

    {
        // test if alignment with offset works correctly too
        core::PoolAllocator allocator(memory, memory + 4096, 20, 32, 4);

        void* alloc0 = allocator.allocate(20, 32, 4);
        X_ASSERT_NOT_NULL(alloc0);
        X_ASSERT_ALIGNMENT(alloc0, 32, 4);
        EXPECT_EQ(allocator.getSize(alloc0), 20);

        void* alloc1 = allocator.allocate(16, 16, 4);
        X_ASSERT_NOT_NULL(alloc1);
        X_ASSERT_ALIGNMENT(alloc1, 16, 4);
        EXPECT_EQ(allocator.getSize(alloc1), 20);

        void* alloc2 = allocator.allocate(20, 8, 4);
        X_ASSERT_NOT_NULL(alloc2);
        X_ASSERT_ALIGNMENT(alloc2, 8, 4);
        EXPECT_EQ(allocator.getSize(alloc2), 20);

        // a pool allocator can free allocations in any order
        allocator.free(alloc1);
        allocator.free(alloc0);
        allocator.free(alloc2);
    }
}

TEST(PoolAlloc, Heap)
{
    core::HeapArea heapArea(core::VirtualMem::GetPageSize());
    X_ASSERT_ALIGNMENT(heapArea.start(), 32, 0);

    for (unsigned int N = 8; N <= 64; N <<= 1) {
        // with an element size of N and a maximum alignment of N and no offset, there should be space for 4096/N allocations
        const unsigned int allocationCount = core::VirtualMem::GetPageSize() / N;
        core::PoolAllocator allocator(heapArea.start(), heapArea.end(), N, N, 0);

        for (unsigned int i = 0; i < allocationCount; ++i) {
            void* alloc = allocator.allocate(N, N, 0);
            X_ASSERT_NOT_NULL(alloc);
            X_ASSERT_ALIGNMENT(alloc, N, 0);
            memset(alloc, 0xAB, N);

            EXPECT_EQ(allocator.getSize(alloc), N);
        }

        core::debugging::EnableBreakpoints(false);

        g_AssetChecker.ExpectAssertion(true);

        // there should be no space left
        void* failedAlloc = allocator.allocate(4, 4, 0);
        EXPECT_TRUE(failedAlloc == nullptr);
        EXPECT_TRUE(g_AssetChecker.HadCorrectAssertions());

        g_AssetChecker.ExpectAssertion(false);

        core::debugging::EnableBreakpoints(true);
    }

    {
        core::PoolAllocator allocator(heapArea.start(), heapArea.end(), 32, 32, 0);

        // making allocations smaller than the element size should be allowed
        void* alloc0 = allocator.allocate(4, 32, 0);
        X_ASSERT_NOT_NULL(alloc0);
        X_ASSERT_ALIGNMENT(alloc0, 32, 0);
        EXPECT_EQ(allocator.getSize(alloc0), 32);

        // making allocations with a smaller alignment should be allowed as well
        void* alloc1 = allocator.allocate(32, 4, 0);
        X_ASSERT_NOT_NULL(alloc1);
        X_ASSERT_ALIGNMENT(alloc1, 4, 0);
        EXPECT_EQ(allocator.getSize(alloc1), 32);

        void* alloc2 = allocator.allocate(4, 4, 0);
        X_ASSERT_NOT_NULL(alloc2);
        X_ASSERT_ALIGNMENT(alloc2, 4, 0);
        EXPECT_EQ(allocator.getSize(alloc2), 32);

#if X_ENABLE_POOL_ALLOCATOR_CHECK
        // larger element sizes or alignments are disallowed

        core::debugging::EnableBreakpoints(false);

        g_AssetChecker.ExpectAssertion(true);
        void* alloc3 = allocator.allocate(40, 4, 0);
        EXPECT_TRUE(g_AssetChecker.HadCorrectAssertions());

        g_AssetChecker.ExpectAssertion(true);
        void* alloc4 = allocator.allocate(16, 64, 0);
        EXPECT_TRUE(g_AssetChecker.HadCorrectAssertions());

        g_AssetChecker.ExpectAssertion(false);
        core::debugging::EnableBreakpoints(true);

        allocator.free(alloc4);
        allocator.free(alloc3);
#endif

        allocator.free(alloc2);
        allocator.free(alloc1);
        allocator.free(alloc0);
    }

    {
        // test if alignment with offset works correctly too
        core::PoolAllocator allocator(heapArea.start(), heapArea.end(), 20, 32, 4);

        void* alloc0 = allocator.allocate(20, 32, 4);
        X_ASSERT_NOT_NULL(alloc0);
        X_ASSERT_ALIGNMENT(alloc0, 32, 4);
        EXPECT_EQ(allocator.getSize(alloc0), 20);

        void* alloc1 = allocator.allocate(16, 16, 4);
        X_ASSERT_NOT_NULL(alloc1);
        X_ASSERT_ALIGNMENT(alloc1, 16, 4);
        EXPECT_EQ(allocator.getSize(alloc1), 20);

        void* alloc2 = allocator.allocate(20, 8, 4);
        X_ASSERT_NOT_NULL(alloc2);
        X_ASSERT_ALIGNMENT(alloc2, 8, 4);
        EXPECT_EQ(allocator.getSize(alloc2), 20);

        // a pool allocator can free allocations in any order
        allocator.free(alloc1);
        allocator.free(alloc0);
        allocator.free(alloc2);
    }
}

#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS

TEST(PoolAlloc, Stats)
{
    X_ALIGNED_SYMBOL(char memory[4096], 32) = {};
    X_ASSERT_ALIGNMENT(memory, 32, 0);

    {
        // in order to allocate 24-byte elements aligned to 16 bytes, the element size internally becomes 32 bytes
        core::PoolAllocator allocator(memory, memory + 4096, 24, 16, 0);

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
            void* alloc = allocator.allocate(24, 16, 0);
            X_ASSERT_NOT_NULL(alloc);
            X_ASSERT_ALIGNMENT(alloc, 16, 0);
            memset(alloc, 0xAB, 24);

            {
                const core::MemoryAllocatorStatistics& stats = allocator.getStatistics();
                EXPECT_EQ(stats.allocationCount_, 1);
                EXPECT_EQ(stats.allocationCountMax_, 1);
                EXPECT_EQ(stats.virtualMemoryReserved_, 4096);
                EXPECT_EQ(stats.physicalMemoryAllocated_, 4096);
                EXPECT_EQ(stats.physicalMemoryAllocatedMax_, 4096);
                EXPECT_EQ(stats.physicalMemoryUsed_, 32);
                EXPECT_EQ(stats.physicalMemoryUsedMax_, 32);
                EXPECT_EQ(stats.wasteAlignment_, 32 - 24);
                EXPECT_EQ(stats.wasteAlignmentMax_, 32 - 24);
                EXPECT_EQ(stats.wasteUnused_, 0);
                EXPECT_EQ(stats.wasteUnusedMax_, 0);
                EXPECT_EQ(stats.internalOverhead_, 0);
                EXPECT_EQ(stats.internalOverheadMax_, 0);
            }

            allocator.free(alloc);

            {
                const core::MemoryAllocatorStatistics& stats = allocator.getStatistics();
                EXPECT_EQ(stats.allocationCount_, 0);
                EXPECT_EQ(stats.allocationCountMax_, 1);
                EXPECT_EQ(stats.virtualMemoryReserved_, 4096);
                EXPECT_EQ(stats.physicalMemoryAllocated_, 4096);
                EXPECT_EQ(stats.physicalMemoryAllocatedMax_, 4096);
                EXPECT_EQ(stats.physicalMemoryUsed_, 0);
                EXPECT_EQ(stats.physicalMemoryUsedMax_, 32);
                EXPECT_EQ(stats.wasteAlignment_, 0);
                EXPECT_EQ(stats.wasteAlignmentMax_, 32 - 24);
                EXPECT_EQ(stats.wasteUnused_, 0);
                EXPECT_EQ(stats.wasteUnusedMax_, 0);
                EXPECT_EQ(stats.internalOverhead_, 0);
                EXPECT_EQ(stats.internalOverheadMax_, 0);
            }
        }
    }

    {
        core::PoolAllocator allocator(memory, memory + 4096, 16, 16, 4);

        {
            const core::MemoryAllocatorStatistics& stats = allocator.getStatistics();
            EXPECT_EQ(stats.allocationCount_, 0);
            EXPECT_EQ(stats.allocationCountMax_, 0);
            EXPECT_EQ(stats.virtualMemoryReserved_, 4096);
            EXPECT_EQ(stats.physicalMemoryAllocated_, 4096);
            EXPECT_EQ(stats.physicalMemoryAllocatedMax_, 4096);

            // the allocator already needs physical memory due to the offset at the front
            EXPECT_EQ(stats.physicalMemoryUsed_, 12);
            EXPECT_EQ(stats.physicalMemoryUsedMax_, 12);
            EXPECT_EQ(stats.wasteAlignment_, 12);
            EXPECT_EQ(stats.wasteAlignmentMax_, 12);

            // because of the offset and the 12 bytes waste at the front, there's an unused space of 4 bytes at the back
            EXPECT_EQ(stats.wasteUnused_, 4);
            EXPECT_EQ(stats.wasteUnusedMax_, 4);
            EXPECT_EQ(stats.internalOverhead_, 0);
            EXPECT_EQ(stats.internalOverheadMax_, 0);
        }

        {
            void* alloc = allocator.allocate(16, 16, 4);
            X_ASSERT_NOT_NULL(alloc);
            X_ASSERT_ALIGNMENT(alloc, 16, 4);
            memset(alloc, 0xAB, 16);

            {
                const core::MemoryAllocatorStatistics& stats = allocator.getStatistics();
                EXPECT_EQ(stats.allocationCount_, 1);
                EXPECT_EQ(stats.allocationCountMax_, 1);
                EXPECT_EQ(stats.virtualMemoryReserved_, 4096);
                EXPECT_EQ(stats.physicalMemoryAllocated_, 4096);
                EXPECT_EQ(stats.physicalMemoryAllocatedMax_, 4096);
                EXPECT_EQ(stats.physicalMemoryUsed_, 12 + 16);
                EXPECT_EQ(stats.physicalMemoryUsedMax_, 12 + 16);
                EXPECT_EQ(stats.wasteAlignment_, 12);
                EXPECT_EQ(stats.wasteAlignmentMax_, 12);
                EXPECT_EQ(stats.wasteUnused_, 4);
                EXPECT_EQ(stats.wasteUnusedMax_, 4);
                EXPECT_EQ(stats.internalOverhead_, 0);
                EXPECT_EQ(stats.internalOverheadMax_, 0);
            }

            allocator.free(alloc);

            {
                const core::MemoryAllocatorStatistics& stats = allocator.getStatistics();
                EXPECT_EQ(stats.allocationCount_, 0);
                EXPECT_EQ(stats.allocationCountMax_, 1);
                EXPECT_EQ(stats.virtualMemoryReserved_, 4096);
                EXPECT_EQ(stats.physicalMemoryAllocated_, 4096);
                EXPECT_EQ(stats.physicalMemoryAllocatedMax_, 4096);
                EXPECT_EQ(stats.physicalMemoryUsed_, 12);
                EXPECT_EQ(stats.physicalMemoryUsedMax_, 12 + 16);
                EXPECT_EQ(stats.wasteAlignment_, 12);
                EXPECT_EQ(stats.wasteAlignmentMax_, 12);
                EXPECT_EQ(stats.wasteUnused_, 4);
                EXPECT_EQ(stats.wasteUnusedMax_, 4);
                EXPECT_EQ(stats.internalOverhead_, 0);
                EXPECT_EQ(stats.internalOverheadMax_, 0);
            }
        }
    }
}

#endif // X_ENABLE_MEMORY_ALLOCATOR_STATISTICS