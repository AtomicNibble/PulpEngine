#pragma once

#ifndef X_SIGNEDTYPES_H_
#define X_SIGNEDTYPES_H_

X_NAMESPACE_BEGIN(core)

/// \ingroup Traits
namespace traits
{
    /// \ingroup Traits
    /// \brief A traits class for deducing the signed type of any type.
    /// \details This class template offers a nested type which corresponds to the signed type of a given built-in type,
    /// e.g. SignedType<unsigned int>::Type will yield <tt>int</tt>.
    ///	\sa UnsignedType
    template<typename T>
    struct SignedType
    {
    };

    /// \brief Template specialization for char types.
    /// \sa SignedType
    template<>
    struct SignedType<char>
    {
        typedef signed char Type;
    };

    /// \brief Template specialization for signed char types.
    /// \sa SignedType
    template<>
    struct SignedType<signed char>
    {
        typedef signed char Type;
    };

    /// \brief Template specialization for unsigned char types.
    /// \sa SignedType
    template<>
    struct SignedType<unsigned char>
    {
        typedef signed char Type;
    };

    /// \brief Template specialization for signed short types.
    /// \sa SignedType
    template<>
    struct SignedType<signed short>
    {
        typedef signed short Type;
    };

    /// \brief Template specialization for unsigned short types.
    /// \sa SignedType
    template<>
    struct SignedType<unsigned short>
    {
        typedef signed short Type;
    };

    /// \brief Template specialization for signed int types.
    /// \sa SignedType
    template<>
    struct SignedType<signed int>
    {
        typedef signed int Type;
    };

    /// \brief Template specialization for unsigned int types.
    /// \sa SignedType
    template<>
    struct SignedType<unsigned int>
    {
        typedef signed int Type;
    };

    /// \brief Template specialization for signed long types.
    /// \sa SignedType
    template<>
    struct SignedType<signed long>
    {
        typedef signed long Type;
    };

    /// \brief Template specialization for unsigned long types.
    /// \sa SignedType
    template<>
    struct SignedType<unsigned long>
    {
        typedef signed long Type;
    };

    /// \brief Template specialization for signed long long types.
    /// \sa SignedType
    template<>
    struct SignedType<signed long long>
    {
        typedef signed long long Type;
    };

    /// \brief Template specialization for unsigned long long types.
    /// \sa SignedType
    template<>
    struct SignedType<unsigned long long>
    {
        typedef signed long long Type;
    };
} // namespace traits

X_NAMESPACE_END

#endif // X_SIGNEDTYPES_H_
