#include "stdafx.h"

#include <Containers\Array.h>
#include <Util\Event.h>

#include <Memory\VirtualMem.h>
#include <Memory\AllocationPolicies\LinearAllocator.h>

X_USING_NAMESPACE;

// event definitions
namespace
{
    typedef core::Event<void(void)> NoArgumentEvent;
    typedef core::Event<void(int)> SingleArgumentEvent;
    typedef core::Event<void(float, int)> ComplexEvent;
} // namespace

// free functions
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
    static void FreeFunctionComplex(float value, int)
    {
        g_floatValue = value;
    }
} // namespace

// member functions
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

        void FunctionComplex(float value, int)
        {
            floatValue_ = value;
        }

        void FunctionComplexConst(float value, int) const
        {
            constFloatValue_ = value;
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
    typedef core::MemoryArena<core::LinearAllocator, core::SingleThreadPolicy, core::NoBoundsChecking,
        core::NoMemoryTracking, core::NoMemoryTagging>
        LinearArena;
}

TEST(Event, Interface)
{
    core::HeapArea heap(core::VirtualMem::GetPageSize());
    core::LinearAllocator allocator(heap.start(), heap.end());
    LinearArena memoryArena(&allocator, "EventSink");

    {
        NoArgumentEvent::Sink sink(&memoryArena, 4);

        NoArgumentEvent testEvent;
        EXPECT_FALSE(testEvent.IsBound());
        testEvent.Bind(&sink);
        EXPECT_TRUE(testEvent.IsBound());
    }
}

TEST(Event, Sink)
{
    core::HeapArea heap(core::VirtualMem::GetPageSize());
    core::LinearAllocator allocator(heap.start(), heap.end());
    LinearArena memoryArena(&allocator, "EventSink");

    {
        NoArgumentEvent::Sink sink(&memoryArena, 4);
        EXPECT_EQ(0, sink.GetListenerCount());
    }
}

TEST(Event, NoArg)
{
    core::HeapArea heap(core::VirtualMem::GetPageSize());
    core::LinearAllocator allocator(heap.start(), heap.end());
    LinearArena memoryArena(&allocator, "EventSink");

    {
        TestClass instance;
        const TestClass constInstance;

        NoArgumentEvent::Sink sink(&memoryArena, 4);
        EXPECT_EQ(0, sink.GetListenerCount());
        sink.AddListener<&FreeFunction>();
        EXPECT_EQ(1, sink.GetListenerCount());
        sink.AddListener<TestClass, &TestClass::Function>(&instance);
        EXPECT_EQ(2, sink.GetListenerCount());
        sink.AddListener<TestClass, &TestClass::FunctionConst>(&instance);
        EXPECT_EQ(3, sink.GetListenerCount());
        sink.AddListener<TestClass, &TestClass::FunctionConst>(&constInstance);
        EXPECT_EQ(4, sink.GetListenerCount());

        NoArgumentEvent testEvent;
        testEvent.Bind(&sink);
        testEvent.Signal();

        EXPECT_TRUE(g_calledFreeFunction);
        EXPECT_TRUE(instance.calledFunction_);
        EXPECT_TRUE(instance.calledConstFunction_);
        EXPECT_TRUE(constInstance.calledConstFunction_);

        EXPECT_EQ(4, sink.GetListenerCount());
        sink.RemoveListener<&FreeFunction>();
        EXPECT_EQ(3, sink.GetListenerCount());
        sink.RemoveListener<TestClass, &TestClass::Function>(&instance);
        EXPECT_EQ(2, sink.GetListenerCount());
        sink.RemoveListener<TestClass, &TestClass::FunctionConst>(&instance);
        EXPECT_EQ(1, sink.GetListenerCount());
        sink.RemoveListener<TestClass, &TestClass::FunctionConst>(&constInstance);
        EXPECT_EQ(0, sink.GetListenerCount());
    }
}

TEST(Event, SingleArg)
{
    core::HeapArea heap(core::VirtualMem::GetPageSize());
    core::LinearAllocator allocator(heap.start(), heap.end());
    LinearArena memoryArena(&allocator, "EventSink");

    {
        TestClass instance;
        const TestClass constInstance;

        SingleArgumentEvent::Sink sink(&memoryArena, 4);
        EXPECT_EQ(0, sink.GetListenerCount());
        sink.AddListener<&FreeFunctionSingleArgument>();
        EXPECT_EQ(1, sink.GetListenerCount());
        sink.AddListener<TestClass, &TestClass::FunctionSingleArgument>(&instance);
        EXPECT_EQ(2, sink.GetListenerCount());
        sink.AddListener<TestClass, &TestClass::FunctionSingleArgumentConst>(&instance);
        EXPECT_EQ(3, sink.GetListenerCount());
        sink.AddListener<TestClass, &TestClass::FunctionSingleArgumentConst>(&constInstance);
        EXPECT_EQ(4, sink.GetListenerCount());

        SingleArgumentEvent testEvent;
        testEvent.Bind(&sink);
        testEvent.Signal(5);

        EXPECT_EQ(5, g_value);
        EXPECT_EQ(5, instance.value_);
        EXPECT_EQ(5, instance.constValue_);
        EXPECT_EQ(5, constInstance.constValue_);

        EXPECT_EQ(4, sink.GetListenerCount());
        sink.RemoveListener<&FreeFunctionSingleArgument>();
        EXPECT_EQ(3, sink.GetListenerCount());
        sink.RemoveListener<TestClass, &TestClass::FunctionSingleArgument>(&instance);
        EXPECT_EQ(2, sink.GetListenerCount());
        sink.RemoveListener<TestClass, &TestClass::FunctionSingleArgumentConst>(&instance);
        EXPECT_EQ(1, sink.GetListenerCount());
        sink.RemoveListener<TestClass, &TestClass::FunctionSingleArgumentConst>(&constInstance);
        EXPECT_EQ(0, sink.GetListenerCount());
    }
}

TEST(Event, MultiArg)
{
    core::HeapArea heap(core::VirtualMem::GetPageSize());
    core::LinearAllocator allocator(heap.start(), heap.end());
    LinearArena memoryArena(&allocator, "EventSink");

    {
        TestClass instance;
        const TestClass constInstance;

        ComplexEvent::Sink sink(&memoryArena, 4);
        EXPECT_EQ(0, sink.GetListenerCount());
        sink.AddListener<&FreeFunctionComplex>();
        EXPECT_EQ(1, sink.GetListenerCount());
        sink.AddListener<TestClass, &TestClass::FunctionComplex>(&instance);
        EXPECT_EQ(2, sink.GetListenerCount());
        sink.AddListener<TestClass, &TestClass::FunctionComplexConst>(&instance);
        EXPECT_EQ(3, sink.GetListenerCount());
        sink.AddListener<TestClass, &TestClass::FunctionComplexConst>(&constInstance);
        EXPECT_EQ(4, sink.GetListenerCount());

        ComplexEvent testEvent;
        testEvent.Bind(&sink);
        testEvent.Signal(5.f, 10);

        EXPECT_EQ(5, g_floatValue);
        EXPECT_EQ(5, instance.floatValue_);
        EXPECT_EQ(5, instance.constFloatValue_);
        EXPECT_EQ(5, constInstance.constFloatValue_);

        EXPECT_EQ(4, sink.GetListenerCount());
        sink.RemoveListener<&FreeFunctionComplex>();
        EXPECT_EQ(3, sink.GetListenerCount());
        sink.RemoveListener<TestClass, &TestClass::FunctionComplex>(&instance);
        EXPECT_EQ(2, sink.GetListenerCount());
        sink.RemoveListener<TestClass, &TestClass::FunctionComplexConst>(&instance);
        EXPECT_EQ(1, sink.GetListenerCount());
        sink.RemoveListener<TestClass, &TestClass::FunctionComplexConst>(&constInstance);
        EXPECT_EQ(0, sink.GetListenerCount());
    }
}
