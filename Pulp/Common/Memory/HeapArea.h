#pragma once

#ifndef X_HEAPAREA_H_
#define X_HEAPAREA_H_

X_NAMESPACE_BEGIN(core)


class HeapArea
{
public:
    // Allocates a block of memory using the virtual memory system.
    // The given size must be a multiple of the virtual memory page size.
    explicit HeapArea(size_t size);

    // Frees all memory allocated by the heap area.
    ~HeapArea(void);

    // Returns the start of the heap area.
    X_INLINE void* start(void) const;

    // Returns the end of the heap area.
    X_INLINE void* end(void) const;

private:
    void* start_;
    void* end_;
};

#include "HeapArea.inl"

X_NAMESPACE_END

#endif
