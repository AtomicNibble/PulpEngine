#pragma once

#ifndef X_UNSIGNEDTYPES_H_
#define X_UNSIGNEDTYPES_H_

X_NAMESPACE_BEGIN(core)

namespace traits
{
    template<typename T>
    struct UnsignedType
    {
    };

    template<>
    struct UnsignedType<char>
    {
        typedef unsigned char Type;
    };

    template<>
    struct UnsignedType<signed char>
    {
        typedef unsigned char Type;
    };

    template<>
    struct UnsignedType<unsigned char>
    {
        typedef unsigned char Type;
    };

    template<>
    struct UnsignedType<signed short>
    {
        typedef unsigned short Type;
    };

    template<>
    struct UnsignedType<unsigned short>
    {
        typedef unsigned short Type;
    };

    template<>
    struct UnsignedType<signed int>
    {
        typedef unsigned int Type;
    };

    template<>
    struct UnsignedType<unsigned int>
    {
        typedef unsigned int Type;
    };

    template<>
    struct UnsignedType<signed long>
    {
        typedef unsigned long Type;
    };

    template<>
    struct UnsignedType<unsigned long>
    {
        typedef unsigned long Type;
    };

    template<>
    struct UnsignedType<signed long long>
    {
        typedef unsigned long long Type;
    };

    template<>
    struct UnsignedType<unsigned long long>
    {
        typedef unsigned long long Type;
    };
} // namespace traits

X_NAMESPACE_END

#endif // X_UNSIGNEDTYPES_H_
