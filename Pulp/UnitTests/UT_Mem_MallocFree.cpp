#include "stdafx.h"



#include <Memory\AllocationPolicies\MallocFreeAllocator.h>
#include <Memory\VirtualMem.h>

X_USING_NAMESPACE;


using namespace core;

TEST(Malloc, Allocate)
{
	core::MallocFreeAllocator allocator;

	for (unsigned int alignment = 4; alignment <= 1024; alignment <<= 1)
	{
		X_LOG0("MallocFreeAlloc", "Allocating with alignment %d", alignment);

		for (unsigned int size = 2048; size <= 4096; size += 8)
		{
			void* alloc = allocator.allocate(size, alignment, 4);
			X_ASSERT_NOT_NULL(alloc);
			X_ASSERT_ALIGNMENT(alloc, alignment, 4);

			ASSERT_TRUE(alloc != NULL);
			ASSERT_TRUE(core::internal::IsAligned(alloc, alignment, 4));

			memset(alloc, 0xAB, size);

			EXPECT_EQ(allocator.getSize(alloc), size);

			allocator.free(alloc);
		}
	}
}