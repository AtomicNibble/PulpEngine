#include "stdafx.h"

#include "Util\Delegate.h"

X_USING_NAMESPACE;

// event definitions
namespace
{
    typedef core::Delegate<void(void)> NoArgumentDelegate;
    typedef core::Delegate<int(void)> ReturnValueDelegate;
    typedef core::Delegate<void(int)> SingleArgumentDelegate;
    typedef core::Delegate<int(float, int)> ComplexDelegate;

} // namespace

namespace
{
    static bool g_calledFreeFunction = false;
    static void FreeFunction(void)
    {
        g_calledFreeFunction = true;
    }

    static int g_value = 0;
    static void FreeFunctionSingleArgument(int value)
    {
        g_value = value;
    }

    static float g_floatValue = 0.0f;
    static int FreeFunctionComplex(float value, int)
    {
        g_floatValue = value;
        return 24;
    }
} // namespace

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

TEST(Delegate, SimpleFree)
{
    NoArgumentDelegate simple;
    simple.Bind<&FreeFunction>();
    simple.Invoke();

    EXPECT_TRUE(g_calledFreeFunction);
}

TEST(Delegate, SimpleMember)
{
    TestClass instance;

    NoArgumentDelegate simple;
    simple.Bind<TestClass, &TestClass::Function>(&instance);
    simple.Invoke();

    EXPECT_TRUE(instance.calledFunction_);
}

TEST(Delegate, SimpleMemberConst)
{
    TestClass instance;
    const TestClass instanceConst;

    {
        NoArgumentDelegate simple;
        simple.Bind<TestClass, &TestClass::FunctionConst>(&instance);
        simple.Invoke();

        EXPECT_TRUE(instance.calledConstFunction_);
    }
    {
        NoArgumentDelegate simple;
        simple.Bind<TestClass, &TestClass::FunctionConst>(&instanceConst);
        simple.Invoke();

        EXPECT_TRUE(instanceConst.calledConstFunction_);
    }
}

TEST(Delegate, SingleArgFree)
{
    SingleArgumentDelegate singleArgDel;
    singleArgDel.Bind<&FreeFunctionSingleArgument>();
    singleArgDel.Invoke(6);

    EXPECT_EQ(6, g_value);
}

TEST(Delegate, SingleArgMember)
{
    TestClass instance;

    SingleArgumentDelegate singleArgDel;
    singleArgDel.Bind<TestClass, &TestClass::FunctionSingleArgument>(&instance);
    singleArgDel.Invoke(8);

    EXPECT_EQ(8, instance.value_);
}

TEST(Delegate, SingleArgMemberConst)
{
    TestClass instance;
    const TestClass instanceConst;

    {
        SingleArgumentDelegate singleArgDel;
        singleArgDel.Bind<TestClass, &TestClass::FunctionSingleArgumentConst>(&instance);
        singleArgDel.Invoke(16);

        EXPECT_EQ(16, instance.constValue_);
    }
    {
        SingleArgumentDelegate singleArgDel;
        singleArgDel.Bind<TestClass, &TestClass::FunctionSingleArgumentConst>(&instanceConst);
        singleArgDel.Invoke(32);

        EXPECT_EQ(32, instanceConst.constValue_);
    }
}

TEST(Delegate, ComplexArgFree)
{
    ComplexDelegate complexgDel;
    complexgDel.Bind<&::FreeFunctionComplex>();

    EXPECT_EQ(24, complexgDel.Invoke(12.f, 6));
    EXPECT_EQ(6, g_value);
}

TEST(Delegate, ComplexArgMember)
{
    TestClass instance;

    ComplexDelegate complexgDel;
    complexgDel.Bind<TestClass, &TestClass::FunctionComplex>(&instance);

    EXPECT_EQ(24, complexgDel.Invoke(16.f, 8));
    EXPECT_FLOAT_EQ(16.f, instance.floatValue_);
}

TEST(Delegate, ComplexMemberConst)
{
    TestClass instance;
    const TestClass instanceConst;

    {
        ComplexDelegate complexgDel;
        complexgDel.Bind<TestClass, &TestClass::FunctionComplexConst>(&instance);

        EXPECT_EQ(24, complexgDel.Invoke(32.f, 16));
        EXPECT_FLOAT_EQ(32.f, instance.constFloatValue_);
    }
    {
        ComplexDelegate complexgDel;
        complexgDel.Bind<TestClass, &TestClass::FunctionComplexConst>(&instanceConst);

        EXPECT_EQ(24, complexgDel.Invoke(64.f, 32));
        EXPECT_FLOAT_EQ(64.f, instanceConst.constFloatValue_);
    }
}
