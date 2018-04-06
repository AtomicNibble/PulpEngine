#include "stdafx.h"

#include <Memory/AllocationPolicies/GrowingMicroAllocator.h>
#include <Memory\VirtualMem.h>

X_USING_NAMESPACE;

using namespace core;

TEST(MicroAlloc, Heap)
{
    uint32_t total_allocs = 0;
    for (unsigned int alignment = 4; alignment <= 64; alignment <<= 1) {
        for (unsigned int size = 4; size <= 256; size <<= 1) {
            for (unsigned int offset = 0; offset <= 4; offset += 4) {
                core::GrowingMicroAllocator allocator(1 * 1024 * 1024, VirtualMem::GetPageSize(), alignment, offset);

                for (unsigned int i = 0; i < 2048; ++i) {
                    void* alloc = allocator.allocate(size, alignment, offset);
                    X_ASSERT_NOT_NULL(alloc);
                    X_ASSERT_ALIGNMENT(alloc, alignment, offset);
                    memset(alloc, 0xAB, size);

                    allocator.free(alloc);

                    total_allocs++;

                    const size_t allocationSize = allocator.getSize(alloc);
                    EXPECT_TRUE(allocationSize >= size && allocationSize <= bitUtil::NextPowerOfTwo(allocationSize));
                    EXPECT_TRUE(allocator.containsAllocation(alloc) == true);
                }
            }
        }
    }
    X_LOG0("MicroAlloc", "Total Allocs: %i", total_allocs);
}