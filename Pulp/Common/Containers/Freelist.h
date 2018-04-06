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
    inline Freelist(void* start, void* end, size_t elementSize, size_t alignment, size_t offset);

    // \brief Obtains an element from the freelist.
    // \remark Returns a \c nullptr if no free entry is available.
    inline void* Obtain(void);

    /// Returns an entry to the list.
    inline void Return(void* ptr);

private:
    Freelist* next_;
};

#include "Freelist.inl"

X_NAMESPACE_END

#endif // !X_CON_FREELIST_H_
