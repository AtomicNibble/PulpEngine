#include "stdafx.h"

#include <Memory/AllocationPolicies/StackAllocator.h>
#include <Memory\AllocationPolicies\GrowingStackAllocator.h>
#include <Memory\VirtualMem.h>

X_USING_NAMESPACE;

using namespace core;

TEST(StackAlloc, Stack)
{
    X_ALIGNED_SYMBOL(char buf[4096], 32) = {};
    X_ASSERT_ALIGNMENT(buf, 32, 0);

    StackAllocator allocator(buf, buf + 4096);

    for (size_t size = 0; size < 64; ++size) {
        for (uint32_t alignment = 1; alignment <= 32; alignment <<= 1) {
            void* ptr = allocator.allocate(size, alignment, 0);

            X_ASSERT_NOT_NULL(ptr);
            X_ASSERT_ALIGNMENT(ptr, alignment, 0);
            ASSERT_TRUE(ptr != NULL);
            ASSERT_TRUE(core::internal::IsAligned(ptr, alignment, 0));
            memset(ptr, 0xAB, size);

            EXPECT_EQ(size, allocator.getSize(ptr));
            allocator.free(ptr);
        }
    }

    {
        void* alloc0 = allocator.allocate(4, 32, 4);
        X_ASSERT_NOT_NULL(alloc0);
        X_ASSERT_ALIGNMENT(alloc0, 32, 4);
        ASSERT_TRUE(alloc0 != NULL);
        ASSERT_TRUE(core::internal::IsAligned(alloc0, 32, 4));
        memset(alloc0, 0xAB, 4);

        // with an offset of 4, the allocation has to leave a gap of (32-4 = 28) bytes.
        // all extra stuff needed internally by the allocator (checks and offsets) fit into the remaining bytes.
        EXPECT_EQ(alloc0, (buf + 28));
        EXPECT_EQ(allocator.getSize(alloc0), 4);

        void* alloc1 = allocator.allocate(16, 16, 0);
        X_ASSERT_NOT_NULL(alloc1);
        X_ASSERT_ALIGNMENT(alloc1, 16, 0);
        ASSERT_TRUE(alloc1 != NULL);
        ASSERT_TRUE(core::internal::IsAligned(alloc1, 16, 0));
        memset(alloc1, 0xAB, 16);

        // the first allocation (4 bytes) must end at a 16-byte boundary, hence this allocation should be at the next boundary
#if X_64 && X_ENABLE_STACK_ALLOCATOR_CHECK
        EXPECT_EQ(buf + 32 + 32, alloc1);
#else
        EXPECT_EQ(buf + 32 + 16, alloc1);
#endif
        EXPECT_EQ(16, allocator.getSize(alloc1));

        allocator.free(alloc1);
        allocator.free(alloc0);
    }

    {
        // we've got space for exactly 4096 bytes, try to allocate as much
#if X_ENABLE_STACK_ALLOCATOR_CHECK
        void* alloc0 = allocator.allocate(4096 - sizeof(uintptr_t) * 3, 4, 0);
        X_ASSERT_NOT_NULL(alloc0);
        X_ASSERT_ALIGNMENT(alloc0, 4, 0);
        ASSERT_TRUE(alloc0 != NULL);
        ASSERT_TRUE(core::internal::IsAligned(alloc0, 4, 0));

        memset(alloc0, 0xAB, 4096 - sizeof(uintptr_t) * 3);
        EXPECT_EQ(allocator.getSize(alloc0), 4096 - sizeof(uintptr_t) * 3);
#else
        void* alloc0 = allocator.allocate(4096 - sizeof(uintptr_t) * 2, 4, 0);
        X_ASSERT_NOT_NULL(alloc0);
        X_ASSERT_ALIGNMENT(alloc0, 4, 0);

        ASSERT_TRUE(alloc0 != NULL);
        ASSERT_TRUE(core::internal::IsAligned(alloc0, 4, 0));

        memset(alloc0, 0xAB, 4096 - sizeof(uintptr_t) * 2);
        EXPECT_EQ(4096 - sizeof(uintptr_t) * 2, allocator.getSize(alloc0));
#endif

        core::debugging::EnableBreakpoints(false);

        // no more space left
        void* failedAlloc = allocator.allocate(4, 4, 0);
        EXPECT_TRUE(failedAlloc == nullptr);
        allocator.free(alloc0);

        core::debugging::EnableBreakpoints(true);
    }

    {
        void* alloc0 = allocator.allocate(1024, 32, 0);
        X_ASSERT_NOT_NULL(alloc0);
        X_ASSERT_ALIGNMENT(alloc0, 32, 0);
        ASSERT_TRUE(alloc0 != NULL);
        ASSERT_TRUE(core::internal::IsAligned(alloc0, 32, 0));

        memset(alloc0, 0xAB, 1024);
        EXPECT_EQ(1024, allocator.getSize(alloc0));

        void* alloc1 = allocator.allocate(1024, 32, 0);
        X_ASSERT_NOT_NULL(alloc1);
        X_ASSERT_ALIGNMENT(alloc1, 32, 0);
        ASSERT_TRUE(alloc1 != NULL);
        ASSERT_TRUE(core::internal::IsAligned(alloc1, 32, 0));

        memset(alloc1, 0xAB, 1024);
        EXPECT_EQ(1024, allocator.getSize(alloc1));

        // the allocator should complain about freeing the allocations in the wrong order
#if X_ENABLE_STACK_ALLOCATOR_CHECK
        core::debugging::EnableBreakpoints(false);

        g_AssetChecker.ExpectAssertion(true);
        allocator.free(alloc0);
        EXPECT_TRUE(g_AssetChecker.HadCorrectAssertions());

        g_AssetChecker.ExpectAssertion(true);
        allocator.free(alloc1);
        EXPECT_TRUE(g_AssetChecker.HadCorrectAssertions());
        g_AssetChecker.ExpectAssertion(false);

        core::debugging::EnableBreakpoints(true);
#endif // !X_ENABLE_STACK_ALLOCATOR_CHECK
    }

    //	X_NEW(int, &arena, "test");
}

TEST(StackAlloc, Heap)
{
    core::HeapArea heapArea(core::VirtualMem::GetPageSize());
    X_ASSERT_ALIGNMENT(heapArea.start(), 32, 0);
    char* memory = static_cast<char*>(heapArea.start());

    core::StackAllocator allocator(heapArea.start(), heapArea.end());

    for (size_t size = 0; size < 64; ++size) {
        for (uint32_t alignment = 1; alignment <= 32; alignment <<= 1) {
            void* alloc = allocator.allocate(size, alignment, 0);
            X_ASSERT_NOT_NULL(alloc);
            X_ASSERT_ALIGNMENT(alloc, alignment, 0);
            ASSERT_TRUE(alloc != NULL);
            ASSERT_TRUE(core::internal::IsAligned(alloc, alignment, 0));

            memset(alloc, 0xAB, size);

            EXPECT_EQ(size, allocator.getSize(alloc));
            allocator.free(alloc);
        }
    }

    {
        void* alloc0 = allocator.allocate(4, 32, 4);
        X_ASSERT_NOT_NULL(alloc0);
        X_ASSERT_ALIGNMENT(alloc0, 32, 4);
        memset(alloc0, 0xAB, 4);

        // with an offset of 4, the allocation has to leave a gap of (32-4 = 28) bytes.
        // all extra stuff needed internally by the allocator (checks and offsets) fit into the remaining bytes.
        EXPECT_EQ(memory + 28, alloc0);
        EXPECT_EQ(4, allocator.getSize(alloc0));

        void* alloc1 = allocator.allocate(16, 16, 0);
        X_ASSERT_NOT_NULL(alloc1);
        X_ASSERT_ALIGNMENT(alloc1, 16, 0);
        memset(alloc1, 0xAB, 16);

        // the first allocation (4 bytes) must end at a 16-byte boundary, hence this allocation should be at the next boundary
#if X_64 && X_ENABLE_STACK_ALLOCATOR_CHECK
        EXPECT_EQ(memory + 32 + 32, alloc1);
#else
        EXPECT_EQ(memory + 32 + 16, alloc1);
#endif
        EXPECT_EQ(16, allocator.getSize(alloc1));

        allocator.free(alloc1);
        allocator.free(alloc0);
    }

    {
        // we've got space for exactly 4096 bytes, try to allocate as much
#if X_ENABLE_STACK_ALLOCATOR_CHECK
        void* alloc0 = allocator.allocate(4096 - sizeof(uintptr_t) * 3, 4, 0);
        X_ASSERT_NOT_NULL(alloc0);
        X_ASSERT_ALIGNMENT(alloc0, 4, 0);
        ASSERT_TRUE(alloc0 != NULL);

        memset(alloc0, 0xAB, 4096 - sizeof(uintptr_t) * 3);
        EXPECT_EQ(4096 - sizeof(uintptr_t) * 3, allocator.getSize(alloc0));
#else
        void* alloc0 = allocator.allocate(4096 - sizeof(uintptr_t) * 2, 4, 0);
        X_ASSERT_NOT_NULL(alloc0);
        X_ASSERT_ALIGNMENT(alloc0, 4, 0);
        ASSERT_TRUE(alloc0 != NULL);
        memset(alloc0, 0xAB, 4096 - sizeof(uintptr_t) * 2);
        EXPECT_EQ(4096 - sizeof(uintptr_t) * 2, allocator.getSize(alloc0));
#endif

        core::debugging::EnableBreakpoints(false);

        // no more space left
        void* failedAlloc = allocator.allocate(4, 4, 0);
        EXPECT_EQ(failedAlloc, nullptr);
        allocator.free(alloc0);

        core::debugging::EnableBreakpoints(true);
    }
}

#if X_ENABLE_MEMORY_ALLOCATOR_STATISTICS

TEST(StackAlloc, Stats)
{
    X_ALIGNED_SYMBOL(char memory[4096], 32) = {};
    X_ASSERT_ALIGNMENT(memory, 32, 0);

    core::StackAllocator allocator(memory, memory + 4096);

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
        ASSERT_TRUE(alloc != NULL);
        ASSERT_TRUE(core::internal::IsAligned(alloc, 32, 0));

        memset(alloc, 0xAB, 100);

        size_t wasteAlignmentMax = 0;
        size_t internalOverheadMax = 0;
        {
            const core::MemoryAllocatorStatistics& stats = allocator.getStatistics();
            EXPECT_EQ(stats.allocationCount_, 1);
            EXPECT_EQ(stats.allocationCountMax_, 1);
            EXPECT_EQ(stats.virtualMemoryReserved_, 4096);
            EXPECT_EQ(stats.physicalMemoryAllocated_, 4096);
            EXPECT_EQ(stats.physicalMemoryAllocatedMax_, 4096);
#if X_64
            EXPECT_EQ(stats.physicalMemoryUsed_, 100 + 32);
            EXPECT_EQ(stats.physicalMemoryUsedMax_, 100 + 32);
#else
            EXPECT_EQ(stats.physicalMemoryUsed_, 100 + 32);
            EXPECT_EQ(stats.physicalMemoryUsedMax_, 100 + 32);
#endif

#if X_ENABLE_STACK_ALLOCATOR_CHECK
            EXPECT_EQ(stats.wasteAlignment_, 32 - sizeof(uintptr_t) * 2 - sizeof(size_t));
            EXPECT_EQ(stats.wasteAlignmentMax_, 32 - sizeof(uintptr_t) * 2 - sizeof(size_t));
            EXPECT_EQ(stats.internalOverhead_, sizeof(uintptr_t) * 2 + sizeof(size_t));
            EXPECT_EQ(stats.internalOverheadMax_, sizeof(uintptr_t) * 2 + sizeof(size_t));
#else
            EXPECT_EQ(stats.wasteAlignment_, 32 - sizeof(uintptr_t) * 1 - sizeof(size_t));
            EXPECT_EQ(stats.wasteAlignmentMax_, 32 - sizeof(uintptr_t) * 1 - sizeof(size_t));
            EXPECT_EQ(stats.internalOverhead_, sizeof(uintptr_t) * 1 + sizeof(size_t));
            EXPECT_EQ(stats.internalOverheadMax_, sizeof(uintptr_t) * 1 + sizeof(size_t));
#endif

            EXPECT_EQ(stats.wasteUnused_, 0);
            EXPECT_EQ(stats.wasteUnusedMax_, 0);

            wasteAlignmentMax = stats.wasteAlignmentMax_;
            internalOverheadMax = stats.internalOverheadMax_;
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
#if X_64
            EXPECT_EQ(stats.physicalMemoryUsedMax_, 100 + 32);
#else
            EXPECT_EQ(stats.physicalMemoryUsedMax_, 100 + 32);
#endif
            EXPECT_EQ(stats.wasteAlignment_, 0);
            EXPECT_EQ(stats.wasteAlignmentMax_, wasteAlignmentMax);
            EXPECT_EQ(stats.wasteUnused_, 0);
            EXPECT_EQ(stats.wasteUnusedMax_, 0);
            EXPECT_EQ(stats.internalOverhead_, 0);
            EXPECT_EQ(stats.internalOverheadMax_, internalOverheadMax);
        }
    }
}

#endif

TEST(StackAllocGrow, Heap)
{
    core::GrowingStackAllocator allocator(16 * 1024 * 1024, 64 * 1024);

    for (unsigned int alignment = 4; alignment <= 64; alignment <<= 1) {
        for (unsigned int size = 4; size <= 512; size <<= 1) {
            for (unsigned int offset = 0; offset <= 4; offset += 4) {
                for (unsigned int i = 0; i < 1024; ++i) {
                    void* alloc = allocator.allocate(size, alignment, offset);
                    X_ASSERT_NOT_NULL(alloc);
                    X_ASSERT_ALIGNMENT(alloc, alignment, offset);

                    ASSERT_TRUE(alloc != NULL);
                    ASSERT_TRUE(core::internal::IsAligned(alloc, alignment, offset));

                    memset(alloc, 0xAB, size);

                    EXPECT_EQ(allocator.getSize(alloc), size);
                }
            }
        }
    }
}

TEST(StackAllocGrow, Purge)
{
    core::GrowingStackAllocator allocator(16 * 1024 * 1024, 64 * 1024);

    for (unsigned int alignment = 4; alignment <= 64; alignment <<= 1) {
        for (unsigned int size = 4; size <= 1 * 1024 * 1024; size <<= 1) {
            for (unsigned int offset = 0; offset <= 4; offset += 4) {
                void* alloc = allocator.allocate(size, alignment, offset);
                X_ASSERT_NOT_NULL(alloc);
                X_ASSERT_ALIGNMENT(alloc, alignment, offset);

                ASSERT_TRUE(alloc != NULL);
                ASSERT_TRUE(core::internal::IsAligned(alloc, alignment, offset));
                memset(alloc, 0xAB, size);

                EXPECT_EQ(allocator.getSize(alloc), size);

                allocator.free(alloc);
                allocator.purge();
            }
        }
    }
}