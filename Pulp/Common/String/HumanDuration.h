#pragma once

#ifndef X_CORE_STRUTIL_HUMAN_DURATION_H_
#define X_CORE_STRUTIL_HUMAN_DURATION_H_

#include "StackString.h"

X_NAMESPACE_BEGIN(core)

/*
Rounds up ms into a sensible representation.
*/
namespace HumanDuration
{
    typedef StackString<64> Str;

    const char* toString(Str& str, float ms);
    const char* toString(Str& str, int64_t ms);

} // namespace HumanDuration

X_NAMESPACE_END

#endif // X_CORE_STRUTIL_HUMAN_DURATION_H_