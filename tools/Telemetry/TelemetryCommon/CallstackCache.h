#pragma once

#include "Types.h"

#include <memory.h>

// This is a fixed sized hash table with linear probing
// That works over an arbitrary block of memory
// Delete is not supported.

struct CallstackCache
{
    tt_uint32* pTable;
    tt_uint32 size;
    tt_uint32 sizeMask;
};

inline CallstackCache CreateCallstackCache(tt_uint8* pBuf, tt_size size)
{
    memset(pBuf, 0, size);

    size = size / sizeof(tt_uint32);

    CallstackCache st;
    st.pTable = reinterpret_cast<tt_uint32*>(pBuf);
    st.size = static_cast<tt_uint32>(size);
    st.sizeMask = st.size - 1;
    return st;
}

inline tt_uint32 getHash(tt_uint32 id)
{
    return id;
}

inline tt_uint32 getIndex(CallstackCache& csc, tt_uint32 id)
{
    auto hash = getHash(id);

    return hash & csc.sizeMask;
}

inline bool CallstackCacheContainsAdd(CallstackCache& csc, tt_uint32 id)
{
    auto index = getIndex(csc, id);

    // zero is sentinal
    while (index < csc.size && csc.pTable[index] != 0u)
    {
        if (csc.pTable[index] == id) {
            return true;
        }

        index = index + 1 & csc.sizeMask;
    }

    // it's not in cache, and cache is full.
    if (index == csc.size) {
        return false;
    }

    csc.pTable[index] = id;
    return false;
}
