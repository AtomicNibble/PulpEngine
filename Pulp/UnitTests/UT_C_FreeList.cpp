#include "stdafx.h"

#include <Memory\VirtualMem.h>

X_USING_NAMESPACE;

using namespace core;

TEST(FreeList, Stack)
{
    char memory[4096] = {};
    for (size_t size = 1; size < 128; ++size) {
        for (uint32_t alignment = 1; alignment <= 64; alignment <<= 1) {
            for (uint32_t offset = 0; offset <= 4; ++offset) {
                Freelist freelist(memory, memory + 4096, size, alignment, 4 * offset);

                char* entries[4] = {};
                for (size_t i = 0; i < 4; ++i) {
                    void* entry = freelist.Obtain();
                    X_ASSERT_NOT_NULL(entry);
                    X_ASSERT_ALIGNMENT(entry, alignment, 4 * offset);
                    memset(entry, static_cast<int>(i), size);

                    entries[i] = static_cast<char*>(entry);
                }

                EXPECT_EQ(0, *entries[0]);
                EXPECT_EQ(1, *entries[1]);
                EXPECT_EQ(2, *entries[2]);
                EXPECT_EQ(3, *entries[3]);

                // the other entries must all still contain valid data after other entries have been returned
                freelist.Return(entries[0]);
                EXPECT_EQ(1, *entries[1]);
                EXPECT_EQ(2, *entries[2]);
                EXPECT_EQ(3, *entries[3]);

                freelist.Return(entries[3]);
                EXPECT_EQ(1, *entries[1]);
                EXPECT_EQ(2, *entries[2]);

                freelist.Return(entries[1]);
                EXPECT_EQ(2, *entries[2]);

                freelist.Return(entries[2]);
            }
        }
    }
}

TEST(FreeList, Heap)
{
    HeapArea heapArea(VirtualMem::GetPageSize());

    for (size_t size = 1; size < 128; ++size) {
        for (uint32_t alignment = 1; alignment <= 64; alignment <<= 1) {
            for (uint32_t offset = 0; offset <= 4; ++offset) {
                Freelist freelist(heapArea.start(), heapArea.end(), size, alignment, 4 * offset);

                // obtain 4 entries, and free them in any order
                char* entries[4] = {};
                for (size_t i = 0; i < 4; ++i) {
                    void* entry = freelist.Obtain();
                    X_ASSERT_NOT_NULL(entry);
                    X_ASSERT_ALIGNMENT(entry, alignment, 4 * offset);
                    memset(entry, static_cast<int>(i), size);

                    entries[i] = static_cast<char*>(entry);
                }

                EXPECT_EQ(0, *entries[0]);
                EXPECT_EQ(1, *entries[1]);
                EXPECT_EQ(2, *entries[2]);
                EXPECT_EQ(3, *entries[3]);

                // the other entries must all still contain valid data after other entries have been returned
                freelist.Return(entries[0]);
                EXPECT_EQ(1, *entries[1]);
                EXPECT_EQ(2, *entries[2]);
                EXPECT_EQ(3, *entries[3]);

                freelist.Return(entries[3]);
                EXPECT_EQ(1, *entries[1]);
                EXPECT_EQ(2, *entries[2]);

                freelist.Return(entries[1]);
                EXPECT_EQ(2, *entries[2]);

                freelist.Return(entries[2]);
            }
        }
    }
}