#pragma once

#ifndef X_DLMALLOC_H
#define X_DLMALLOC_H


extern "C"
{

	typedef void* mspace;

	/// \ingroup dlmalloc
	/// \brief Creates a memory space.
	extern mspace create_mspace(size_t capacity, int locked);

	/// \ingroup dlmalloc
	/// \brief Creates a memory space containing a pre-allocated region of memory.
	extern mspace create_mspace_with_base(void* base, size_t capacity, int locked);

	/// \ingroup dlmalloc
	/// \brief Destroys a memory space.
	extern size_t destroy_mspace(mspace msp);


	/// \ingroup dlmalloc
	/// \brief Allocates raw memory from a memory space.
	extern void* mspace_malloc(mspace msp, size_t bytes);

	/// \ingroup dlmalloc
	/// \brief Frees memory in a memory space.
	extern void mspace_free(mspace msp, void* mem);

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
