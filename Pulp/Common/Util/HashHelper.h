#pragma once

#include <Hashing\Fnva1Hash.h>
#include <String\StrRef.h>

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
struct hash<core::string>
{
    size_t operator()(const core::string& __s) const
    {
        return (size_t)core::Hash::Fnv1aHash(__s.data(), __s.length());
    }

    size_t operator()(const core::Path<char>& __s) const
    {
        return (size_t)core::Hash::Fnv1aHash(__s.data(), __s.length());
    }

    size_t operator()(const char* const __s) const
    {
        return (size_t)core::Hash::Fnv1aHash(__s, strlen(__s));
    }
};

template<size_t N>
struct hash<core::StackString<N>>
{
    size_t operator()(const core::StackString<N>& __s) const
    {
        return (size_t)core::Hash::Fnv1aHash(__s.c_str(), __s.length());
    }

    size_t operator()(const core::Path<char>& __s) const
    {
        return (size_t)core::Hash::Fnv1aHash(__s.data(), __s.length());
    }

    size_t operator()(const core::string& __s) const
    {
        return (size_t)core::Hash::Fnv1aHash(__s.data(), __s.length());
    }
};

template<typename CharT>
struct hash<core::Path<CharT>>
{
    size_t operator()(const core::Path<CharT>& __s) const
    {
        return (size_t)core::Hash::Fnv1aHash(__s.c_str(), __s.length());
    }
};

template<>
struct hash<const char*>
{
    size_t operator()(const char* const __s) const
    {
        return (size_t)core::Hash::Fnv1aHash(__s, strlen(__s));
    }

    size_t operator()(const core::string& __s) const
    {
        return (size_t)core::Hash::Fnv1aHash(__s.data(), __s.length());
    }
};

template<>
struct hash<const char* const>
{
    size_t operator()(const char* const __s) const
    {
        return (size_t)core::Hash::Fnv1aHash(__s, strlen(__s));
    }
};

// -------------------------------------------------------------------

template<class _Type = void>
struct equal_to
{
    bool operator()(const _Type& _Left, const _Type& _Right) const
    {
        return (_Left == _Right);
    }
};

template<>
struct equal_to<core::string>
{
    bool operator()(const core::string& _Left, const core::string& _Right) const
    {
        return (_Left == _Right);
    }

    bool operator()(const core::string& _Left, const core::Path<char>& _Right) const
    {
        return core::strUtil::IsEqual(_Left.begin(), _Left.end(), _Right.begin(), _Right.end());
    }

    bool operator()(const core::string& _Left, const char* const _Right) const
    {
        return _Left.compare(_Right);
    }
};

template<size_t N>
struct equal_to<core::StackString<N>>
{
    bool operator()(const core::StackString<N>& _Left, const core::StackString<N>& _Right) const
    {
        return (_Left == _Right);
    }

    bool operator()(const core::StackString<N>& _Left, core::Path<char>& _Right) const
    {
        return core::strUtil::IsEqual(_Left.begin(), _Left.end(), _Right.begin(), _Right.end());
    }

    bool operator()(const core::StackString<N>& _Left, const core::string& _Right) const
    {
        return core::strUtil::IsEqual(_Left.begin(), _Left.end(), _Right.begin(), _Right.end());
    }
};

template<typename CharT>
struct equal_to<core::Path<CharT>>
{
    bool operator()(const core::Path<CharT>& _Left, const core::Path<CharT>& _Right) const
    {
        return (_Left == _Right);
    }
};

template<>
struct equal_to<const char*>
{
    bool operator()(const char* const _Left, const char* const _Right) const
    {
        return core::strUtil::IsEqual(_Left, _Right);
    }

    bool operator()(const char* const _Left, const core::string& _Right) const
    {
        return core::strUtil::IsEqual(_Right.begin(), _Right.end(), _Left);
    }
};

template<>
struct equal_to<const char* const>
{
    bool operator()(const char* const _Left, const char* const _Right) const
    {
        return core::strUtil::IsEqual(_Left, _Right);
    }

    bool operator()(const char* const _Left, const core::string& _Right) const
    {
        return core::strUtil::IsEqual(_Right.begin(), _Right.end(), _Left);
    }
};


X_NAMESPACE_END