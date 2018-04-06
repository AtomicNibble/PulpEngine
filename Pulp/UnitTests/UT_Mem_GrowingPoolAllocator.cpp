#include "stdafx.h"

#include <Memory/AllocationPolicies/GrowingPoolAllocator.h>
#include <Memory\VirtualMem.h>

X_USING_NAMESPACE;

using namespace core;

TEST(GrowingPoolAlloc, Heap)
{
    for (unsigned int alignment = 4; alignment <= 64; alignment <<= 1) {
        for (unsigned int size = 4; size <= 512; size <<= 1) {
            for (unsigned int offset = 0; offset <= 4; offset += 4) {
                core::GrowingPoolAllocator allocator(16 * 1024 * 1024, 64 * 1024, 0, size, alignment, offset);

                for (unsigned int i = 0; i < 16384; ++i) {
                    void* alloc = allocator.allocate(size, alignment, offset);
                    X_ASSERT_NOT_NULL(alloc);
                    X_ASSERT_ALIGNMENT(alloc, alignment, offset);
                    memset(alloc, 0xAB, size);

                    EXPECT_EQ(size, allocator.getSize(alloc));
                }
            }
        }
    }
}