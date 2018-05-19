#pragma once
#ifndef X_STRINGRANGE_H
#define X_STRINGRANGE_H

#include "String/StringUtil.h"

X_NAMESPACE_BEGIN(core)

template<typename TChar = char>
class StringRange
{
public:
    inline StringRange(const TChar* const startInclusive, const TChar* const endExclusive);

    inline StringRange(const TChar* const startInclusive, size_t length);

    inline const TChar* find(TChar character) const;
    inline const TChar* find(const TChar* sub) const;

    inline const TChar* findWhitespace(void) const;
    inline const TChar* findNonWhitespace(void) const;

    inline TChar operator[](size_t i) const;

    inline const TChar* getStart(void) const;
    inline const TChar* getEnd(void) const;
    inline size_t getLength(void) const;

    inline const TChar* begin(void) const;
    inline const TChar* end(void) const;

private:
    const TChar* start_;
    const TChar* end_;
};

#include "StringRange.inl"

X_NAMESPACE_END

#endif
