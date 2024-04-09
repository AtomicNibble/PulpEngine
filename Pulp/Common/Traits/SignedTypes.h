#pragma once

#ifndef X_SIGNEDTYPES_H_
#define X_SIGNEDTYPES_H_

X_NAMESPACE_BEGIN(core)

namespace traits
{
    template<typename T>
    struct SignedType
    {
    };

    template<>
    struct SignedType<char>
    {
        typedef signed char Type;
    };

    template<>
    struct SignedType<signed char>
    {
        typedef signed char Type;
    };

    template<>
    struct SignedType<unsigned char>
    {
        typedef signed char Type;
    };

    template<>
    struct SignedType<signed short>
    {
        typedef signed short Type;
    };

    template<>
    struct SignedType<unsigned short>
    {
        typedef signed short Type;
    };

    template<>
    struct SignedType<signed int>
    {
        typedef signed int Type;
    };

    template<>
    struct SignedType<unsigned int>
    {
        typedef signed int Type;
    };

    template<>
    struct SignedType<signed long>
    {
        typedef signed long Type;
    };

    template<>
    struct SignedType<unsigned long>
    {
        typedef signed long Type;
    };

    template<>
    struct SignedType<signed long long>
    {
        typedef signed long long Type;
    };

    template<>
    struct SignedType<unsigned long long>
    {
        typedef signed long long Type;
    };
} // namespace traits

X_NAMESPACE_END

#endif // X_SIGNEDTYPES_H_
