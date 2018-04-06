#pragma once

#ifndef X_CORE_STRUTIL_HUMAN_SIZE_H_
#define X_CORE_STRUTIL_HUMAN_SIZE_H_

#include "StackString.h"

X_NAMESPACE_BEGIN(core)

/*
Turns a number of bytes into a string, mainly used for stats.
will round to a sensible representation aka: bytes, kb, mb etc.
*/
namespace HumanSize
{
    // 20 bytes needed for any number. (plane)
    // only 10 needed for string since we 1024 / 1024
    typedef StackString<28> Str; // 28 + 4 size = 32;

    const char* toString(Str& str, uint32_t numBytes);
    const char* toString(Str& str, uint64_t numBytes);
    const char* toString(Str& str, int64_t numBytes);

} // namespace HumanSize

X_NAMESPACE_END

#endif // X_CORE_STRUTIL_HUMAN_SIZE_H_