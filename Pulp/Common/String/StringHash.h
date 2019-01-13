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

    X_INLINE constexpr StrHash();
    X_INLINE constexpr StrHash(const StrHash& oth) = default;

    template<typename T>
    X_INLINE StrHash(const T& str);
    X_INLINE StrHash(const char* pStr, size_t length); /// Constructs a StringHash from a string with a certain length.
    X_INLINE StrHash(const char* pBegin, const char* pEnd);
    X_INLINE constexpr explicit StrHash(Type hash);

    X_INLINE constexpr StrHash& operator=(const StrHash& oth) = default;

    X_INLINE constexpr operator Type(void) const; /// Cast operator, returning the string's hash.
    X_INLINE constexpr Type hash(void) const;

private:
    Type hash_;
};

namespace Literals
{
    inline constexpr StrHash operator"" _strhash(const char* const pStr, const size_t strLen)
    {
        return StrHash(Hash::Fnv1aConst::Internal::Hash(pStr, strLen, Hash::Fnv1aConst::default_offset_basis));
    }
} // namespace Literals

#include "StringHash.inl"

X_NAMESPACE_END

#endif // _H_STRING_HASH_