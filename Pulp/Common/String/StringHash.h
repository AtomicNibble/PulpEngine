#pragma once

#ifndef _H_STRING_HASH_
#define _H_STRING_HASH_

#include <CompileTime/IntToType.h>
#include <CompileTime/IsStringLiteral.h>
#include <Hashing/Fnva1Hash.h>

X_NAMESPACE_BEGIN(core)

class StrHash
{
public:
    typedef uint32_t Type;

    X_INLINE StrHash();
    X_INLINE StrHash(const StrHash& oth);

    template<typename T>
    X_INLINE StrHash(const T& str);
    X_INLINE StrHash(const char* pStr, size_t length); /// Constructs a StringHash from a string with a certain length.
    X_INLINE StrHash(const char* pBegin, const char* pEnd);
    X_INLINE explicit StrHash(Type hash);

    X_INLINE operator Type(void) const; /// Cast operator, returning the string's hash.

private:
    Type hash_;
};

#include "StringHash.inl"

X_NAMESPACE_END

#endif // _H_STRING_HASH_