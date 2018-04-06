#pragma once

#ifndef X_UNSIGNEDTYPES_H_
#define X_UNSIGNEDTYPES_H_

X_NAMESPACE_BEGIN(core)

/// \ingroup Traits
namespace traits
{
    /// \ingroup Traits
    /// \brief A traits class for deducing the unsigned type of any type.
    /// \details This class template offers a nested type which corresponds to the unsigned type of a given built-in type,
    /// e.g. UnsignedType<signed int>::Type will yield <tt>unsigned int</tt>.
    ///	\sa SignedType
    template<typename T>
    struct UnsignedType
    {
    };

    /// \brief Template specialization for char types.
    /// \sa UnsignedType
    template<>
    struct UnsignedType<char>
    {
        typedef unsigned char Type;
    };

    /// \brief Template specialization for signed char types.
    /// \sa UnsignedType
    template<>
    struct UnsignedType<signed char>
    {
        typedef unsigned char Type;
    };

    /// \brief Template specialization for unsigned char types.
    /// \sa UnsignedType
    template<>
    struct UnsignedType<unsigned char>
    {
        typedef unsigned char Type;
    };

    /// \brief Template specialization for signed short types.
    /// \sa UnsignedType
    template<>
    struct UnsignedType<signed short>
    {
        typedef unsigned short Type;
    };

    /// \brief Template specialization for unsigned short types.
    /// \sa UnsignedType
    template<>
    struct UnsignedType<unsigned short>
    {
        typedef unsigned short Type;
    };

    /// \brief Template specialization for signed int types.
    /// \sa UnsignedType
    template<>
    struct UnsignedType<signed int>
    {
        typedef unsigned int Type;
    };

    /// \brief Template specialization for unsigned int types.
    /// \sa UnsignedType
    template<>
    struct UnsignedType<unsigned int>
    {
        typedef unsigned int Type;
    };

    /// \brief Template specialization for signed long types.
    /// \sa UnsignedType
    template<>
    struct UnsignedType<signed long>
    {
        typedef unsigned long Type;
    };

    /// \brief Template specialization for unsigned long types.
    /// \sa UnsignedType
    template<>
    struct UnsignedType<unsigned long>
    {
        typedef unsigned long Type;
    };

    /// \brief Template specialization for signed long long types.
    /// \sa UnsignedType
    template<>
    struct UnsignedType<signed long long>
    {
        typedef unsigned long long Type;
    };

    /// \brief Template specialization for unsigned long long types.
    /// \sa UnsignedType
    template<>
    struct UnsignedType<unsigned long long>
    {
        typedef unsigned long long Type;
    };
} // namespace traits

X_NAMESPACE_END

#endif // X_UNSIGNEDTYPES_H_
