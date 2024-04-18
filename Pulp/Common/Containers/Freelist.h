#pragma once

#ifndef X_CON_FREELIST_H_
#define X_CON_FREELIST_H_

X_NAMESPACE_BEGIN(core)

class Freelist
{
public:
    // Default constructor, leaves the freelist empty.
    inline Freelist(void);

    // Constructs a freelist in the memory range [start, end).
    inline Freelist(void* pStart, void* pEnd, size_t elementSize, size_t alignment, size_t offset);

    // Obtains an element from the freelist.
    // Returns a nullptr if no free entry is available.
    inline void* Obtain(void);

    // Returns an entry to the list.
    inline void Return(void* pPtr);

private:
    Freelist* next_;
};

#include "Freelist.inl"

X_NAMESPACE_END

#endif // X_CON_FREELIST_H_
