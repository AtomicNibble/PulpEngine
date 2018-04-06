#include "stdafx.h"

#include <Containers\FixedFifo.h>

#include <Traits\SignedTypes.h>
#include <Traits\UnsignedTypes.h>
#include <Traits\FunctionTraits.h>

X_USING_NAMESPACE;

using namespace core;

namespace
{
    template<typename T, typename U>
    struct SameT
    {
        static const bool Value = false;
    };

    template<typename T>
    struct SameT<T, T>
    {
        static const bool Value = true;
    };
} // namespace

TEST(Traits, Signed)
{
    EXPECT_TRUE((SameT<traits::SignedType<unsigned char>::Type, signed char>::Value));
    EXPECT_TRUE((SameT<traits::SignedType<signed char>::Type, signed char>::Value));
    EXPECT_TRUE((SameT<traits::SignedType<char>::Type, signed char>::Value));

    EXPECT_TRUE((SameT<traits::SignedType<unsigned short>::Type, signed short>::Value));
    EXPECT_TRUE((SameT<traits::SignedType<signed short>::Type, signed short>::Value));
    EXPECT_TRUE((SameT<traits::SignedType<short>::Type, signed short>::Value));

    EXPECT_TRUE((SameT<traits::SignedType<unsigned int>::Type, signed int>::Value));
    EXPECT_TRUE((SameT<traits::SignedType<signed int>::Type, signed int>::Value));
    EXPECT_TRUE((SameT<traits::SignedType<int>::Type, signed int>::Value));

    EXPECT_TRUE((SameT<traits::SignedType<unsigned long>::Type, signed long>::Value));
    EXPECT_TRUE((SameT<traits::SignedType<signed long>::Type, signed long>::Value));
    EXPECT_TRUE((SameT<traits::SignedType<long>::Type, signed long>::Value));

    EXPECT_TRUE((SameT<traits::SignedType<unsigned long long>::Type, signed long long>::Value));
    EXPECT_TRUE((SameT<traits::SignedType<signed long long>::Type, signed long long>::Value));
    EXPECT_TRUE((SameT<traits::SignedType<long long>::Type, signed long long>::Value));
}

TEST(Traits, UnSigned)
{
    EXPECT_TRUE((SameT<traits::UnsignedType<unsigned char>::Type, unsigned char>::Value));
    EXPECT_TRUE((SameT<traits::UnsignedType<signed char>::Type, unsigned char>::Value));
    EXPECT_TRUE((SameT<traits::UnsignedType<char>::Type, unsigned char>::Value));

    EXPECT_TRUE((SameT<traits::UnsignedType<unsigned short>::Type, unsigned short>::Value));
    EXPECT_TRUE((SameT<traits::UnsignedType<signed short>::Type, unsigned short>::Value));
    EXPECT_TRUE((SameT<traits::UnsignedType<short>::Type, unsigned short>::Value));

    EXPECT_TRUE((SameT<traits::UnsignedType<unsigned int>::Type, unsigned int>::Value));
    EXPECT_TRUE((SameT<traits::UnsignedType<signed int>::Type, unsigned int>::Value));
    EXPECT_TRUE((SameT<traits::UnsignedType<int>::Type, unsigned int>::Value));

    EXPECT_TRUE((SameT<traits::UnsignedType<unsigned long>::Type, unsigned long>::Value));
    EXPECT_TRUE((SameT<traits::UnsignedType<signed long>::Type, unsigned long>::Value));
    EXPECT_TRUE((SameT<traits::UnsignedType<long>::Type, unsigned long>::Value));

    EXPECT_TRUE((SameT<traits::UnsignedType<unsigned long long>::Type, unsigned long long>::Value));
    EXPECT_TRUE((SameT<traits::UnsignedType<signed long long>::Type, unsigned long long>::Value));
    EXPECT_TRUE((SameT<traits::UnsignedType<long long>::Type, unsigned long long>::Value));
}

TEST(Traits, Function)
{
    {
        typedef traits::Function<void(void)> TestFunction;
        typedef TestFunction::Pointer TestFunctionPointer;

        EXPECT_TRUE((SameT<TestFunction::ReturnType, void>::Value));
    }

    // arg1
    {
        typedef traits::Function<void(int)> TestFunction;
        typedef TestFunction::Pointer TestFunctionPointer;

        EXPECT_TRUE((SameT<TestFunction::ReturnType, void>::Value));
        EXPECT_TRUE((SameT<TestFunction::Arg0, int>::Value));
    }

    // arg2
    {
        typedef traits::Function<void(int, double)> TestFunction;
        typedef TestFunction::Pointer TestFunctionPointer;

        EXPECT_TRUE((SameT<TestFunction::ReturnType, void>::Value));
        EXPECT_TRUE((SameT<TestFunction::Arg0, int>::Value));
        EXPECT_TRUE((SameT<TestFunction::Arg1, double>::Value));
    }

    // arg3
    {
        typedef traits::Function<bool(int, float, long)> TestFunction;
        typedef TestFunction::Pointer TestFunctionPointer;

        EXPECT_TRUE((SameT<TestFunction::ReturnType, bool>::Value));
        EXPECT_TRUE((SameT<TestFunction::Arg0, int>::Value));
        EXPECT_TRUE((SameT<TestFunction::Arg1, float>::Value));
        EXPECT_TRUE((SameT<TestFunction::Arg2, long>::Value));
    }

    // arg4
    {
        typedef traits::Function<int(int, float, long, int)> TestFunction;
        typedef TestFunction::Pointer TestFunctionPointer;

        EXPECT_TRUE((SameT<TestFunction::ReturnType, int>::Value));
        EXPECT_TRUE((SameT<TestFunction::Arg0, int>::Value));
        EXPECT_TRUE((SameT<TestFunction::Arg1, float>::Value));
        EXPECT_TRUE((SameT<TestFunction::Arg2, long>::Value));
        EXPECT_TRUE((SameT<TestFunction::Arg3, int>::Value));
    }

    // arg5
    {
        typedef traits::Function<void(int, float, long, int, long long)> TestFunction;
        typedef TestFunction::Pointer TestFunctionPointer;

        EXPECT_TRUE((SameT<TestFunction::ReturnType, void>::Value));
        EXPECT_TRUE((SameT<TestFunction::Arg0, int>::Value));
        EXPECT_TRUE((SameT<TestFunction::Arg1, float>::Value));
        EXPECT_TRUE((SameT<TestFunction::Arg2, long>::Value));
        EXPECT_TRUE((SameT<TestFunction::Arg3, int>::Value));
        EXPECT_TRUE((SameT<TestFunction::Arg4, long long>::Value));
    }

    // arg6
    {
        typedef traits::Function<char(int, float, long, int, int, short)> TestFunction;
        typedef TestFunction::Pointer TestFunctionPointer;

        EXPECT_TRUE((SameT<TestFunction::ReturnType, char>::Value));
        EXPECT_TRUE((SameT<TestFunction::Arg0, int>::Value));
        EXPECT_TRUE((SameT<TestFunction::Arg1, float>::Value));
        EXPECT_TRUE((SameT<TestFunction::Arg2, long>::Value));
        EXPECT_TRUE((SameT<TestFunction::Arg3, int>::Value));
        EXPECT_TRUE((SameT<TestFunction::Arg4, int>::Value));
        EXPECT_TRUE((SameT<TestFunction::Arg5, short>::Value));
    }
}
