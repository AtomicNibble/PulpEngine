#pragma once

#ifndef X_MEMORYARENABASE_H_
#define X_MEMORYARENABASE_H_

#include "Memory/MemoryArenaStatistics.h"

#include <Containers\FixedArray.h>

X_NAMESPACE_BEGIN(core)


class MemoryArenaBase
{
	static const int MAX_ARENA_CHILDREN = 16;
public:
	/// Empty destructor.
	virtual ~MemoryArenaBase(void) {}

	/// \brief Allocates a certain amount of raw memory.
	/// \details Most of the parameters expected by this function are passed by either the \ref X_NEW or \ref X_NEW_ARRAY macros.
	/// The \a ID of an allocation is a human-readable string that is used to identify an allocation, and the \a typeName is
	/// a human-readable string that denotes the type of an allocation.
	/// \remark The returned pointer will always adhere to the following alignment requirements: <tt>((ptr + offset) % alignment) == 0</tt>.
	virtual void* allocate( size_t size, size_t alignment, size_t offset, const char* ID, const char* typeName, const SourceInfo& sourceInfo ) X_ABSTRACT;

	/// Frees memory previously allocated by Allocate().
	virtual void free(void* ptr) X_ABSTRACT;
	virtual void free(void* ptr, size_t size) X_ABSTRACT;

	/// Frees memory previously allocated by Allocate().
	virtual size_t getSize(void* ptr) X_ABSTRACT;

	/// Returns statistics regarding the allocations made by the memory arena.
	virtual MemoryArenaStatistics getStatistics(void) const X_ABSTRACT;

	virtual MemoryAllocatorStatistics getAllocatorStatistics(bool children = false) const X_ABSTRACT;

	virtual bool isThreadSafe(void) const X_ABSTRACT;

	// adds it baby.
	void addChildArena(MemoryArenaBase* arena) {
		if (children_.size() == MAX_ARENA_CHILDREN)
			X_WARNING("Memory", "can't add child arena exceeded max: %i", MAX_ARENA_CHILDREN);
		else
			children_.append(arena);
	}

protected:

	core::FixedArray<MemoryArenaBase*, MAX_ARENA_CHILDREN> children_;
};

X_NAMESPACE_END


#endif // X_MEMORYARENABASE_H_
