#pragma once

#include <Hashing\Fnva1Hash.h>
#include <String\StrRef.h>
#include <String\StringHash.h>

X_NAMESPACE_BEGIN(core)

inline size_t _Hash_seq(const unsigned char* _First, size_t _Count)
{
    return (size_t)Hash::Fnv1aHash(_First, _Count);
}

template<class _Ktype>
struct _Bitwise_hash
{
    size_t operator()(const _Ktype& _Keyval) const
    {
        return (_Hash_seq((const unsigned char*)&_Keyval, sizeof(_Ktype)));
    }
};

template<class _Ktype>
struct hash : public _Bitwise_hash<_Ktype>
{
    static const bool _Value = __is_enum(_Ktype);
};

template<>
struct hash<core::StrHash>
{
    constexpr size_t operator()(const core::StrHash str) const
    {
        return (size_t)str.hash();
    }
};


template<>
struct hash<core::string>
{
    size_t operator()(const core::string& str) const
    {
        return (size_t)core::Hash::Fnv1aHash(str.data(), str.length());
    }

    size_t operator()(const core::Path<char>& str) const
    {
        return (size_t)core::Hash::Fnv1aHash(str.data(), str.length());
    }

    size_t operator()(const char* const str) const
    {
        return (size_t)core::Hash::Fnv1aHash(str, strlen(str));
    }

    size_t operator()(const core::string_view& str) const
    {
        return (size_t)core::Hash::Fnv1aHash(str.data(), str.length());
    }
};

template<size_t N>
struct hash<core::StackString<N>>
{
    size_t operator()(const core::StackString<N>& str) const
    {
        return (size_t)core::Hash::Fnv1aHash(str.c_str(), str.length());
    }

    size_t operator()(const core::Path<char>& str) const
    {
        return (size_t)core::Hash::Fnv1aHash(str.data(), str.length());
    }

    size_t operator()(const core::string& str) const
    {
        return (size_t)core::Hash::Fnv1aHash(str.data(), str.length());
    }

    size_t operator()(const core::string_view& str) const
    {
        return (size_t)core::Hash::Fnv1aHash(str.data(), str.length());
    }
};

template<typename CharT>
struct hash<core::Path<CharT>>
{
    size_t operator()(const core::Path<CharT>& str) const
    {
        return (size_t)core::Hash::Fnv1aHash(str.c_str(), str.length());
    }
};

template<>
struct hash<core::string_view>
{
    size_t operator()(const core::string_view& str) const
    {
        return (size_t)core::Hash::Fnv1aHash(str.data(), str.length());
    }

    size_t operator()(const core::string& str) const
    {
        return (size_t)core::Hash::Fnv1aHash(str.data(), str.length());
    }

    size_t operator()(const char* const str) const
    {
        return (size_t)core::Hash::Fnv1aHash(str, strlen(str));
    }
};

template<>
struct hash<const char*>
{
    size_t operator()(const char* const str) const
    {
        return (size_t)core::Hash::Fnv1aHash(str, strlen(str));
    }

    size_t operator()(const core::string& str) const
    {
        return (size_t)core::Hash::Fnv1aHash(str.data(), str.length());
    }

    size_t operator()(const core::string_view& str) const
    {
        return (size_t)core::Hash::Fnv1aHash(str.data(), str.length());
    }
};

template<>
struct hash<const char* const>
{
    size_t operator()(const char* const str) const
    {
        return (size_t)core::Hash::Fnv1aHash(str, strlen(str));
    }
};

// -------------------------------------------------------------------

template<class _Type = void>
struct equal_to
{
    bool operator()(const _Type& lhs, const _Type& rhs) const
    {
        return (lhs == rhs);
    }
};

template<>
struct equal_to<core::StrHash>
{
    constexpr bool operator()(const core::StrHash lhs, const core::StrHash rhs) const
    {
        return (lhs == rhs);
    }
};

template<>
struct equal_to<core::string>
{
    bool operator()(const core::string& lhs, const core::string& rhs) const
    {
        return (lhs == rhs);
    }

    bool operator()(const core::string& lhs, const core::Path<char>& rhs) const
    {
        return core::strUtil::IsEqual(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
    }

    bool operator()(const core::string& lhs, const char* const rhs) const
    {
        return lhs.compare(rhs);
    }

    bool operator()(const core::string& lhs, const core::string_view& rhs) const
    {
        return core::strUtil::IsEqual(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
    }
};


template<size_t N>
struct equal_to<core::StackString<N>>
{
    bool operator()(const core::StackString<N>& lhs, const core::StackString<N>& rhs) const
    {
        return (lhs == rhs);
    }

    bool operator()(const core::StackString<N>& lhs, core::Path<char>& rhs) const
    {
        return core::strUtil::IsEqual(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
    }

    bool operator()(const core::StackString<N>& lhs, const core::string& rhs) const
    {
        return core::strUtil::IsEqual(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
    }

    bool operator()(const core::StackString<N>& lhs, const core::string_view& rhs) const
    {
        return core::strUtil::IsEqual(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
    }
};

template<typename CharT>
struct equal_to<core::Path<CharT>>
{
    bool operator()(const core::Path<CharT>& lhs, const core::Path<CharT>& rhs) const
    {
        return (lhs == rhs);
    }
};

template<>
struct equal_to<core::string_view>
{
    bool operator()(const core::string_view& lhs, const core::string_view& rhs) const
    {
        return core::strUtil::IsEqual(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
    }

    bool operator()(const core::string_view& lhs, const core::string& rhs) const
    {
        return core::strUtil::IsEqual(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
    }

    bool operator()(const core::string_view& lhs, const char* const rhs) const
    {
        return core::strUtil::IsEqual(lhs.begin(), lhs.end(), rhs);
    }
};

template<>
struct equal_to<const char*>
{
    bool operator()(const char* const lhs, const char* const rhs) const
    {
        return core::strUtil::IsEqual(lhs, rhs);
    }

    bool operator()(const char* const lhs, const core::string& rhs) const
    {
        return core::strUtil::IsEqual(rhs.begin(), rhs.end(), lhs);
    }

    bool operator()(const char* const lhs, const core::string_view& rhs) const
    {
        return core::strUtil::IsEqual(rhs.begin(), rhs.end(), lhs);
    }
};

template<>
struct equal_to<const char* const>
{
    bool operator()(const char* const lhs, const char* const rhs) const
    {
        return core::strUtil::IsEqual(lhs, rhs);
    }

    bool operator()(const char* const lhs, const core::string& rhs) const
    {
        return core::strUtil::IsEqual(rhs.begin(), rhs.end(), lhs);
    }

    bool operator()(const char* const lhs, const core::string_view& rhs) const
    {
        return core::strUtil::IsEqual(rhs.begin(), rhs.end(), lhs);
    }
};


template<class _Type = void>
struct equal_to_case_insen
{
};

template<>
struct equal_to_case_insen<core::string>
{
    bool operator()(const core::string& lhs, const core::string& rhs) const
    {
        return lhs.compareCaseInsen(rhs);
    }

    bool operator()(const core::string& lhs, const core::Path<char>& rhs) const
    {
        return core::strUtil::IsEqualCaseInsen(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
    }

    bool operator()(const core::string& lhs, const char* const rhs) const
    {
        return lhs.compareCaseInsen(rhs);
    }

    bool operator()(const core::string& lhs, const core::string_view& rhs) const
    {
        return core::strUtil::IsEqualCaseInsen(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
    }
};

template<>
struct equal_to_case_insen<core::string_view>
{
    bool operator()(const core::string_view& lhs, const core::string_view& rhs) const
    {
        return core::strUtil::IsEqualCaseInsen(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
    }

    bool operator()(const core::string_view& lhs, const core::string& rhs) const
    {
        return core::strUtil::IsEqualCaseInsen(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
    }

    bool operator()(const core::string_view& lhs, const char* const rhs) const
    {
        return core::strUtil::IsEqualCaseInsen(lhs.begin(), lhs.end(), rhs);
    }
};

template<>
struct equal_to_case_insen<const char*>
{
    bool operator()(const char* const lhs, const char* const rhs) const
    {
        return core::strUtil::IsEqualCaseInsen(lhs, rhs);
    }

    bool operator()(const char* const lhs, const core::string& rhs) const
    {
        return core::strUtil::IsEqualCaseInsen(rhs.begin(), rhs.end(), lhs);
    }

    bool operator()(const char* const lhs, const core::string_view& rhs) const
    {
        return core::strUtil::IsEqualCaseInsen(rhs.begin(), rhs.end(), lhs);
    }
};

template<>
struct equal_to_case_insen<const char* const>
{
    bool operator()(const char* const lhs, const char* const rhs) const
    {
        return core::strUtil::IsEqualCaseInsen(lhs, rhs);
    }

    bool operator()(const char* const lhs, const core::string& rhs) const
    {
        return core::strUtil::IsEqualCaseInsen(rhs.begin(), rhs.end(), lhs);
    }

    bool operator()(const char* const lhs, const core::string_view& rhs) const
    {
        return core::strUtil::IsEqualCaseInsen(rhs.begin(), rhs.end(), lhs);
    }
};


X_NAMESPACE_END