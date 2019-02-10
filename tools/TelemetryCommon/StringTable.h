#pragma once

#include "Types.h"

#include <memory.h>

// TODO: see what the compilers generates.

struct StringTableIndex
{
    tt_uint16 inserted : 1;
    tt_uint16 index : 15;
};

static_assert(sizeof(StringTableIndex) == 2, "Incorrect size");

inline StringTableIndex BuildIndex(bool inserted, tt_uintptr index)
{
    return { inserted, static_cast<tt_uint16>(index & 0x7FFF) };
}

// TODO: if this string table is always the same size just make the mask a constant.
// then can skip some loads.
struct StringTable
{
    const void** pTable;
    tt_uint32 size;
    tt_uint32 sizeMask;
};

inline StringTable CreateStringTable(tt_uint8* pBuf, tt_size size)
{
    memset(pBuf, 0, size);

    size = size / sizeof(void*);

    StringTable st;
    st.pTable = reinterpret_cast<const void**>(pBuf);
    st.size = static_cast<tt_uint32>(size);
    st.sizeMask = st.size - 1;
    return st;
}

inline tt_uintptr getHash(const void* pPtr)
{
    tt_uintptr val = reinterpret_cast<tt_uintptr>(pPtr);

    return val;
}

inline tt_uintptr getIndex(StringTable& st, const void* pPtr)
{
    auto hash = getHash(pPtr);

    return hash & st.sizeMask;
}

inline bool StringTableContains(StringTable& st, const void* pPtr)
{
    auto index = getIndex(st, pPtr);

    while (st.pTable[index] != nullptr)
    {
        if (st.pTable[index] == pPtr) {
            return true;
        }

        index = index + 1 & st.sizeMask;
    }

    return false;
}

inline void StringTableInsert(StringTable& st, const void* pPtr)
{
    auto index = getIndex(st, pPtr);

    while (st.pTable[index] != nullptr)
    {
        index = (index + 1) & st.sizeMask;
    }

    st.pTable[index] = pPtr;
}

// Returns true if inserted
inline StringTableIndex StringTableGetIndex(StringTable& st, const void* pPtr)
{
    auto index = getIndex(st, pPtr);

    while (st.pTable[index] != nullptr)
    {
        if (st.pTable[index] == pPtr) {
            return BuildIndex(false, index);
        }

        index = (index + 1) & st.sizeMask;
    }

    st.pTable[index] = pPtr;
    return BuildIndex(true, index);
}
