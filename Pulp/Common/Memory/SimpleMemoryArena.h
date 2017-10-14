#pragma once

#ifndef X_SIMPLEMEMORYARENA_H
#define X_SIMPLEMEMORYARENA_H

#include "Memory/MemoryArenaBase.h"


X_NAMESPACE_BEGIN(core)

template <class AllocationPolicy>
class SimpleMemoryArena : public MemoryArenaBase
{
public:
	/// A simple typedef that introduces the template type into this class.
	typedef AllocationPolicy AllocationPolicy;

	static const bool IS_THREAD_SAFE = false;

public:
	SimpleMemoryArena(AllocationPolicy* allocator, const char* name);

	/// Empty destructor.
	virtual ~SimpleMemoryArena(void);

	/// Allocates raw memory that satisfies the alignment requirements.
	virtual void* allocate(size_t size, size_t alignment, size_t offset
		X_MEM_HUMAN_IDS_CB(const char* ID)
		X_MEM_HUMAN_IDS_CB(const char* typeName)
		X_SOURCE_INFO_MEM_CB(const SourceInfo& sourceInfo)) X_FINAL;

	/// Frees memory previously allocated by Allocate().
	virtual void free(void* ptr) X_FINAL;
	virtual void free(void* ptr, size_t size) X_FINAL;

	virtual size_t getSize(void* ptr) X_FINAL;

	virtual size_t usableSize(void* ptr) const X_FINAL;

	/// Returns statistics regarding the allocations made by the memory arena.
	virtual MemoryArenaStatistics getStatistics(void) const X_FINAL;
	virtual MemoryAllocatorStatistics getAllocatorStatistics(bool children = false) const X_FINAL;

	virtual bool isThreadSafe(void) const X_FINAL;

	/// Returns the memory requirement for an allocation of \a size, taking into account possible overhead.
	static constexpr inline size_t getMemoryRequirement(size_t size);

	/// Returns the alignment requirement for an allocation requiring \a alignment, taking into account possible overhead.
	static constexpr inline size_t getMemoryAlignmentRequirement(size_t alignment);

	/// Returns the offset requirement for an allocation, taking into account possible overhead.
	static constexpr inline size_t getMemoryOffsetRequirement(void);

private:
	X_NO_COPY(SimpleMemoryArena);
	X_NO_ASSIGN(SimpleMemoryArena);

	AllocationPolicy* allocator_;
	const char* name_;

#if X_ENABLE_MEMORY_ARENA_STATISTICS
	MemoryArenaStatistics statistics_;
#endif
};

#include "SimpleMemoryArena.inl"

X_NAMESPACE_END


#endif
