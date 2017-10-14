#pragma once

#ifndef X_DLMALLOC_H
#define X_DLMALLOC_H


extern "C"
{

	typedef void* mspace;

	extern mspace create_mspace(size_t capacity, int locked);

	extern mspace create_mspace_with_base(void* base, size_t capacity, int locked);

	extern size_t destroy_mspace(mspace msp);


	extern void* mspace_malloc(mspace msp, size_t bytes);

	extern void* mspace_memalign(mspace msp, size_t alignment, size_t bytes);

	extern void mspace_free(mspace msp, void* mem);

	extern size_t mspace_usable_size(const void* mem);

	struct mallinfo
	{
		size_t arena;
		size_t ordblks;
		size_t smblks;
		size_t hblks;
		size_t hblkhd;
		size_t usmblks;
		size_t fsmblks;
		size_t uordblks;
		size_t fordblks;
		size_t keepcost;
	};

	/// \ingroup dlmalloc
	/// \brief Returns information about allocations in a memory space.
	extern struct mallinfo mspace_mallinfo(mspace msp);
};


#endif
