#pragma once

#ifndef X_HEAPAREA_H_
#define X_HEAPAREA_H_

X_NAMESPACE_BEGIN(core)

/// \ingroup Memory
/// \brief A simple helper class that allocates a block of memory on the heap.
/// \details The heap area allocates a block of memory directly from the OS. It is mostly used in conjunction with
/// non-growing allocators which satisfy their allocation requests by carving memory out of this pre-allocated block.
/// The following code exemplifies this situation:
/// \code
///   // create a heap area that allocates 64 KB of memory from the heap
///   core::HeapArea heap(64*1024);
///
///   // the linear allocator does all its allocations inside this 64 KB block
///   core::LinearAllocator allocator(heap.GetStart(), heap.GetEnd());
/// \endcode
/// Of course, the heap area can be used with any allocator.
class HeapArea
{
public:
    /// \brief Allocates a block of memory using the virtual memory system.
    /// \remark The given \a size must be a multiple of the virtual memory page size.
    /// \sa virtualMemory
    explicit HeapArea(size_t size);

    /// Frees all memory allocated by the heap area.
    ~HeapArea(void);

    /// Returns the start of the heap area.
    X_INLINE void* start(void) const;

    /// Returns the end of the heap area.
    X_INLINE void* end(void) const;

private:
    void* start_;
    void* end_;
};

#include "HeapArea.inl"

X_NAMESPACE_END

#endif
