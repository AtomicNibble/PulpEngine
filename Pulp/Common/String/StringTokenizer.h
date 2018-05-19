
#pragma once
#ifndef X_STRINGTOKENIZER_H
#define X_STRINGTOKENIZER_H

#include "String/StringRange.h"

X_NAMESPACE_BEGIN(core)


template<typename TChar = char>
class StringTokenizer
{
public:
    StringTokenizer(const TChar* startInclusive, const TChar* endExclusive, TChar delimiter);

    bool extractToken(StringRange<TChar>& range);

private:
    const TChar* start_;
    const TChar* end_;
    TChar delimiter_;
};

X_NAMESPACE_END

#endif
