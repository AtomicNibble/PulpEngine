#pragma once

#ifndef X_SIMPLEMEMORYARENA_H
#define X_SIMPLEMEMORYARENA_H

#include "Memory/MemoryArenaBase.h"


X_NAMESPACE_BEGIN(core)

/// \ingroup Memory
/// \brief Implementation of a memory arena.
/// \details This class implements the MemoryArenaBase interface, and defers allocation requests to an allocator.
/// Unlike the MemoryArena, this arena does not offer any debugging facilities, and can be considered a simple wrapper around
/// an allocator in order to act like an arena.
/// \sa MemoryArenaBase MemoryArena X_NEW X_NEW_ARRAY X_DELETE X_DELETE_ARRAY
template <class AllocationPolicy>
class SimpleMemoryArena : public MemoryArenaBase
{
public:
	/// \brief Constructs an arena that uses a given allocator for satisfying allocation requests.
	/// \details The \a name argument can be used to identify different memory arenas.
	/// \remark Ownership of the provided allocator stays at the calling site.
	/// \remark The given \a name must be a constant string/string literal.
	SimpleMemoryArena(AllocationPolicy* allocator, const char* name);

	/// Empty destructor.
	virtual ~SimpleMemoryArena(void);

	/// Allocates raw memory that satisfies the alignment requirements.
	virtual void* Allocate(size_t size, size_t alignment, size_t offset, const char* ID, const char* typeName, const SourceInfo& sourceInfo) X_OVERRIDE;

	/// Frees memory previously allocated by Allocate().
	virtual void Free(void* ptr) X_OVERRIDE;

	/// Returns statistics regarding the allocations made by the memory arena.
	virtual MemoryArenaStatistics GetStatistics(void) const X_OVERRIDE;

	/// Returns the memory requirement for an allocation of \a size, taking into account possible overhead.
	static inline size_t GetMemoryRequirement(size_t size);

	/// Returns the alignment requirement for an allocation requiring \a alignment, taking into account possible overhead.
	static inline size_t GetMemoryAlignmentRequirement(size_t alignment);

	/// Returns the offset requirement for an allocation, taking into account possible overhead.
	static inline size_t GetMemoryOffsetRequirement(void);

private:
	X_NO_COPY(SimpleMemoryArena);
	X_NO_ASSIGN(SimpleMemoryArena);

	AllocationPolicy* m_allocator;
	const char* m_name;

#if X_ENABLE_MEMORY_ARENA_STATISTICS
	MemoryArenaStatistics m_statistics;
#endif
};

#include "SimpleMemoryArena.inl"

X_NAMESPACE_END


#endif
