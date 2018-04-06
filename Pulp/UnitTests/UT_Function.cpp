#include "stdafx.h"

#include "Util\Function.h"

X_USING_NAMESPACE;

namespace
{
    struct TestClass
    {
        TestClass(void) :
            calledFunction_(false),
            calledConstFunction_(false),
            value_(0),
            constValue_(0),
            floatValue_(0.0f),
            constFloatValue_(0.0f)
        {
        }

        void Function(void)
        {
            calledFunction_ = true;
        }

        void FunctionConst(void) const
        {
            calledConstFunction_ = true;
        }

        void FunctionSingleArgument(int value)
        {
            value_ = value;
        }

        void FunctionSingleArgumentConst(int value) const
        {
            constValue_ = value;
        }

        int FunctionComplex(float value, int)
        {
            floatValue_ = value;
            return 24;
        }

        int FunctionComplexConst(float value, int) const
        {
            constFloatValue_ = value;
            return 24;
        }

        bool calledFunction_;
        mutable bool calledConstFunction_;
        int value_;
        mutable int constValue_;
        float floatValue_;
        mutable float constFloatValue_;
    };

} // namespace

namespace
{
    typedef core::Function<void(void), 32> NoArgumentFunction;
    typedef core::Function<int(void), 32> ReturnValueFunction;
    typedef core::Function<void(int), 32> SingleArgumentFunction;
    typedef core::Function<int(float, int), 48> ComplexFunction;

} // namespace

TEST(Function, SimpleMember)
{
    TestClass instance;

    NoArgumentFunction simple;
    simple = std::bind(&TestClass::Function, &instance);
    simple.Invoke();

    EXPECT_TRUE(instance.calledFunction_);
}

TEST(Function, SimpleLambada)
{
    ReturnValueFunction simpleLam([&]() {
        return 1;
    });
    const int32_t val = simpleLam.Invoke();

    EXPECT_EQ(1, val);
}

TEST(Function, SimpleLambadaRef)
{
    int val = 0;

    NoArgumentFunction simpleLamRef([&]() {
        val = 1;
    });
    simpleLamRef.Invoke();

    EXPECT_EQ(1, val);
}
