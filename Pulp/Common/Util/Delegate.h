
#pragma once



#pragma once














#pragma once












#pragma once






































#pragma once




































#pragma once






























X_NAMESPACE_BEGIN(core)

template <typename T>
class Delegate {};















template<typename R  >
class Delegate<R()>
{
    union InstancePtr
    {
        inline InstancePtr(void) :
            as_void(nullptr)
        {
        }

        inline bool operator==(const InstancePtr& other) const
        {
            return (as_void == other.as_void);
        }

        void* as_void;
        const void* as_const_void;
    };

    typedef traits::Function<R(InstancePtr  )> InternalFunction;

    struct Stub
    {
        inline Stub(void) :
            instance(),
            pFunction(nullptr)
        {
        }

        inline bool operator==(const Stub& other) const
        {
            return ((instance == other.instance) && (pFunction == other.pFunction));
        }

        InstancePtr instance;
        typename InternalFunction::Pointer pFunction;
    };

    template<R (*Function)()>
    static X_INLINE R FunctionStub(InstancePtr  )
    {
        return (Function)();
    }

    template<class C, R (C::*Function)()>
    static X_INLINE R ClassMethodStub(InstancePtr instance  )
    {
        return (static_cast<C*>(instance.as_void)->*Function)();
    }

    template<class C, R (C::*Function)() const>
    static X_INLINE R ConstClassMethodStub(InstancePtr instance  )
    {
        return (static_cast<const C*>(instance.as_const_void)->*Function)();
    }

public:
    template<class C, R (C::*Function)()>
    struct NonConstWrapper
    {
        NonConstWrapper(C* pInstance) :
            pInstance_(pInstance)
        {
        }

        C* pInstance_;
    };

    template<class C, R (C::*Function)() const>
    struct ConstWrapper
    {
        ConstWrapper(const C* pInstance) :
            pInstance_(pInstance)
        {
        }

        const C* pInstance_;
    };

    Delegate(void) :
        stub_()
    {
    }

    template<R (*Function)()>
    void Bind(void)
    {
        stub_.instance.as_void = nullptr;
        stub_.pFunction = &FunctionStub<Function>;
    }

    template<class C, R (C::*Function)()>
    void Bind(NonConstWrapper<C, Function> wrapper)
    {
        stub_.instance.as_void = wrapper.pInstance_;
        stub_.pFunction = &ClassMethodStub<C, Function>;
    }

    template<class C, R (C::*Function)() const>
    void Bind(ConstWrapper<C, Function> wrapper)
    {
        stub_.instance.as_const_void = wrapper.pInstance_;
        stub_.pFunction = &ConstClassMethodStub<C, Function>;
    }

    R Invoke() const
    {
        X_ASSERT(stub_.pFunction != nullptr, "Cannot invoke unbound delegate. Call Bind() first.")();
        return stub_.pFunction(stub_.instance  );
    }

    X_INLINE operator bool() const
    {
        return stub_.pFunction != nullptr;
    }

private:
    Stub stub_;
};


























template<typename R , typename ARG0>
class Delegate<R(ARG0)>
{
    union InstancePtr
    {
        inline InstancePtr(void) :
            as_void(nullptr)
        {
        }

        inline bool operator==(const InstancePtr& other) const
        {
            return (as_void == other.as_void);
        }

        void* as_void;
        const void* as_const_void;
    };

    typedef traits::Function<R(InstancePtr , ARG0)> InternalFunction;

    struct Stub
    {
        inline Stub(void) :
            instance(),
            pFunction(nullptr)
        {
        }

        inline bool operator==(const Stub& other) const
        {
            return ((instance == other.instance) && (pFunction == other.pFunction));
        }

        InstancePtr instance;
        typename InternalFunction::Pointer pFunction;
    };

    template<R (*Function)(ARG0)>
    static X_INLINE R FunctionStub(InstancePtr , ARG0 arg0)
    {
        return (Function)(arg0);
    }

    template<class C, R (C::*Function)(ARG0)>
    static X_INLINE R ClassMethodStub(InstancePtr instance , ARG0 arg0)
    {
        return (static_cast<C*>(instance.as_void)->*Function)(arg0);
    }

    template<class C, R (C::*Function)(ARG0) const>
    static X_INLINE R ConstClassMethodStub(InstancePtr instance , ARG0 arg0)
    {
        return (static_cast<const C*>(instance.as_const_void)->*Function)(arg0);
    }

public:
    template<class C, R (C::*Function)(ARG0)>
    struct NonConstWrapper
    {
        NonConstWrapper(C* pInstance) :
            pInstance_(pInstance)
        {
        }

        C* pInstance_;
    };

    template<class C, R (C::*Function)(ARG0) const>
    struct ConstWrapper
    {
        ConstWrapper(const C* pInstance) :
            pInstance_(pInstance)
        {
        }

        const C* pInstance_;
    };

    Delegate(void) :
        stub_()
    {
    }

    template<R (*Function)(ARG0)>
    void Bind(void)
    {
        stub_.instance.as_void = nullptr;
        stub_.pFunction = &FunctionStub<Function>;
    }

    template<class C, R (C::*Function)(ARG0)>
    void Bind(NonConstWrapper<C, Function> wrapper)
    {
        stub_.instance.as_void = wrapper.pInstance_;
        stub_.pFunction = &ClassMethodStub<C, Function>;
    }

    template<class C, R (C::*Function)(ARG0) const>
    void Bind(ConstWrapper<C, Function> wrapper)
    {
        stub_.instance.as_const_void = wrapper.pInstance_;
        stub_.pFunction = &ConstClassMethodStub<C, Function>;
    }

    R Invoke(ARG0 arg0) const
    {
        X_ASSERT(stub_.pFunction != nullptr, "Cannot invoke unbound delegate. Call Bind() first.")();
        return stub_.pFunction(stub_.instance , arg0);
    }

    X_INLINE operator bool() const
    {
        return stub_.pFunction != nullptr;
    }

private:
    Stub stub_;
};


























template<typename R , typename ARG0, typename ARG1>
class Delegate<R(ARG0, ARG1)>
{
    union InstancePtr
    {
        inline InstancePtr(void) :
            as_void(nullptr)
        {
        }

        inline bool operator==(const InstancePtr& other) const
        {
            return (as_void == other.as_void);
        }

        void* as_void;
        const void* as_const_void;
    };

    typedef traits::Function<R(InstancePtr , ARG0, ARG1)> InternalFunction;

    struct Stub
    {
        inline Stub(void) :
            instance(),
            pFunction(nullptr)
        {
        }

        inline bool operator==(const Stub& other) const
        {
            return ((instance == other.instance) && (pFunction == other.pFunction));
        }

        InstancePtr instance;
        typename InternalFunction::Pointer pFunction;
    };

    template<R (*Function)(ARG0, ARG1)>
    static X_INLINE R FunctionStub(InstancePtr , ARG0 arg0, ARG1 arg1)
    {
        return (Function)(arg0, arg1);
    }

    template<class C, R (C::*Function)(ARG0, ARG1)>
    static X_INLINE R ClassMethodStub(InstancePtr instance , ARG0 arg0, ARG1 arg1)
    {
        return (static_cast<C*>(instance.as_void)->*Function)(arg0, arg1);
    }

    template<class C, R (C::*Function)(ARG0, ARG1) const>
    static X_INLINE R ConstClassMethodStub(InstancePtr instance , ARG0 arg0, ARG1 arg1)
    {
        return (static_cast<const C*>(instance.as_const_void)->*Function)(arg0, arg1);
    }

public:
    template<class C, R (C::*Function)(ARG0, ARG1)>
    struct NonConstWrapper
    {
        NonConstWrapper(C* pInstance) :
            pInstance_(pInstance)
        {
        }

        C* pInstance_;
    };

    template<class C, R (C::*Function)(ARG0, ARG1) const>
    struct ConstWrapper
    {
        ConstWrapper(const C* pInstance) :
            pInstance_(pInstance)
        {
        }

        const C* pInstance_;
    };

    Delegate(void) :
        stub_()
    {
    }

    template<R (*Function)(ARG0, ARG1)>
    void Bind(void)
    {
        stub_.instance.as_void = nullptr;
        stub_.pFunction = &FunctionStub<Function>;
    }

    template<class C, R (C::*Function)(ARG0, ARG1)>
    void Bind(NonConstWrapper<C, Function> wrapper)
    {
        stub_.instance.as_void = wrapper.pInstance_;
        stub_.pFunction = &ClassMethodStub<C, Function>;
    }

    template<class C, R (C::*Function)(ARG0, ARG1) const>
    void Bind(ConstWrapper<C, Function> wrapper)
    {
        stub_.instance.as_const_void = wrapper.pInstance_;
        stub_.pFunction = &ConstClassMethodStub<C, Function>;
    }

    R Invoke(ARG0 arg0, ARG1 arg1) const
    {
        X_ASSERT(stub_.pFunction != nullptr, "Cannot invoke unbound delegate. Call Bind() first.")();
        return stub_.pFunction(stub_.instance , arg0, arg1);
    }

    X_INLINE operator bool() const
    {
        return stub_.pFunction != nullptr;
    }

private:
    Stub stub_;
};


























template<typename R , typename ARG0, typename ARG1, typename ARG2>
class Delegate<R(ARG0, ARG1, ARG2)>
{
    union InstancePtr
    {
        inline InstancePtr(void) :
            as_void(nullptr)
        {
        }

        inline bool operator==(const InstancePtr& other) const
        {
            return (as_void == other.as_void);
        }

        void* as_void;
        const void* as_const_void;
    };

    typedef traits::Function<R(InstancePtr , ARG0, ARG1, ARG2)> InternalFunction;

    struct Stub
    {
        inline Stub(void) :
            instance(),
            pFunction(nullptr)
        {
        }

        inline bool operator==(const Stub& other) const
        {
            return ((instance == other.instance) && (pFunction == other.pFunction));
        }

        InstancePtr instance;
        typename InternalFunction::Pointer pFunction;
    };

    template<R (*Function)(ARG0, ARG1, ARG2)>
    static X_INLINE R FunctionStub(InstancePtr , ARG0 arg0, ARG1 arg1, ARG2 arg2)
    {
        return (Function)(arg0, arg1, arg2);
    }

    template<class C, R (C::*Function)(ARG0, ARG1, ARG2)>
    static X_INLINE R ClassMethodStub(InstancePtr instance , ARG0 arg0, ARG1 arg1, ARG2 arg2)
    {
        return (static_cast<C*>(instance.as_void)->*Function)(arg0, arg1, arg2);
    }

    template<class C, R (C::*Function)(ARG0, ARG1, ARG2) const>
    static X_INLINE R ConstClassMethodStub(InstancePtr instance , ARG0 arg0, ARG1 arg1, ARG2 arg2)
    {
        return (static_cast<const C*>(instance.as_const_void)->*Function)(arg0, arg1, arg2);
    }

public:
    template<class C, R (C::*Function)(ARG0, ARG1, ARG2)>
    struct NonConstWrapper
    {
        NonConstWrapper(C* pInstance) :
            pInstance_(pInstance)
        {
        }

        C* pInstance_;
    };

    template<class C, R (C::*Function)(ARG0, ARG1, ARG2) const>
    struct ConstWrapper
    {
        ConstWrapper(const C* pInstance) :
            pInstance_(pInstance)
        {
        }

        const C* pInstance_;
    };

    Delegate(void) :
        stub_()
    {
    }

    template<R (*Function)(ARG0, ARG1, ARG2)>
    void Bind(void)
    {
        stub_.instance.as_void = nullptr;
        stub_.pFunction = &FunctionStub<Function>;
    }

    template<class C, R (C::*Function)(ARG0, ARG1, ARG2)>
    void Bind(NonConstWrapper<C, Function> wrapper)
    {
        stub_.instance.as_void = wrapper.pInstance_;
        stub_.pFunction = &ClassMethodStub<C, Function>;
    }

    template<class C, R (C::*Function)(ARG0, ARG1, ARG2) const>
    void Bind(ConstWrapper<C, Function> wrapper)
    {
        stub_.instance.as_const_void = wrapper.pInstance_;
        stub_.pFunction = &ConstClassMethodStub<C, Function>;
    }

    R Invoke(ARG0 arg0, ARG1 arg1, ARG2 arg2) const
    {
        X_ASSERT(stub_.pFunction != nullptr, "Cannot invoke unbound delegate. Call Bind() first.")();
        return stub_.pFunction(stub_.instance , arg0, arg1, arg2);
    }

    X_INLINE operator bool() const
    {
        return stub_.pFunction != nullptr;
    }

private:
    Stub stub_;
};


























template<typename R , typename ARG0, typename ARG1, typename ARG2, typename ARG3>
class Delegate<R(ARG0, ARG1, ARG2, ARG3)>
{
    union InstancePtr
    {
        inline InstancePtr(void) :
            as_void(nullptr)
        {
        }

        inline bool operator==(const InstancePtr& other) const
        {
            return (as_void == other.as_void);
        }

        void* as_void;
        const void* as_const_void;
    };

    typedef traits::Function<R(InstancePtr , ARG0, ARG1, ARG2, ARG3)> InternalFunction;

    struct Stub
    {
        inline Stub(void) :
            instance(),
            pFunction(nullptr)
        {
        }

        inline bool operator==(const Stub& other) const
        {
            return ((instance == other.instance) && (pFunction == other.pFunction));
        }

        InstancePtr instance;
        typename InternalFunction::Pointer pFunction;
    };

    template<R (*Function)(ARG0, ARG1, ARG2, ARG3)>
    static X_INLINE R FunctionStub(InstancePtr , ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3)
    {
        return (Function)(arg0, arg1, arg2, arg3);
    }

    template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3)>
    static X_INLINE R ClassMethodStub(InstancePtr instance , ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3)
    {
        return (static_cast<C*>(instance.as_void)->*Function)(arg0, arg1, arg2, arg3);
    }

    template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3) const>
    static X_INLINE R ConstClassMethodStub(InstancePtr instance , ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3)
    {
        return (static_cast<const C*>(instance.as_const_void)->*Function)(arg0, arg1, arg2, arg3);
    }

public:
    template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3)>
    struct NonConstWrapper
    {
        NonConstWrapper(C* pInstance) :
            pInstance_(pInstance)
        {
        }

        C* pInstance_;
    };

    template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3) const>
    struct ConstWrapper
    {
        ConstWrapper(const C* pInstance) :
            pInstance_(pInstance)
        {
        }

        const C* pInstance_;
    };

    Delegate(void) :
        stub_()
    {
    }

    template<R (*Function)(ARG0, ARG1, ARG2, ARG3)>
    void Bind(void)
    {
        stub_.instance.as_void = nullptr;
        stub_.pFunction = &FunctionStub<Function>;
    }

    template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3)>
    void Bind(NonConstWrapper<C, Function> wrapper)
    {
        stub_.instance.as_void = wrapper.pInstance_;
        stub_.pFunction = &ClassMethodStub<C, Function>;
    }

    template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3) const>
    void Bind(ConstWrapper<C, Function> wrapper)
    {
        stub_.instance.as_const_void = wrapper.pInstance_;
        stub_.pFunction = &ConstClassMethodStub<C, Function>;
    }

    R Invoke(ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3) const
    {
        X_ASSERT(stub_.pFunction != nullptr, "Cannot invoke unbound delegate. Call Bind() first.")();
        return stub_.pFunction(stub_.instance , arg0, arg1, arg2, arg3);
    }

    X_INLINE operator bool() const
    {
        return stub_.pFunction != nullptr;
    }

private:
    Stub stub_;
};


























template<typename R , typename ARG0, typename ARG1, typename ARG2, typename ARG3, typename ARG4>
class Delegate<R(ARG0, ARG1, ARG2, ARG3, ARG4)>
{
    union InstancePtr
    {
        inline InstancePtr(void) :
            as_void(nullptr)
        {
        }

        inline bool operator==(const InstancePtr& other) const
        {
            return (as_void == other.as_void);
        }

        void* as_void;
        const void* as_const_void;
    };

    typedef traits::Function<R(InstancePtr , ARG0, ARG1, ARG2, ARG3, ARG4)> InternalFunction;

    struct Stub
    {
        inline Stub(void) :
            instance(),
            pFunction(nullptr)
        {
        }

        inline bool operator==(const Stub& other) const
        {
            return ((instance == other.instance) && (pFunction == other.pFunction));
        }

        InstancePtr instance;
        typename InternalFunction::Pointer pFunction;
    };

    template<R (*Function)(ARG0, ARG1, ARG2, ARG3, ARG4)>
    static X_INLINE R FunctionStub(InstancePtr , ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4)
    {
        return (Function)(arg0, arg1, arg2, arg3, arg4);
    }

    template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4)>
    static X_INLINE R ClassMethodStub(InstancePtr instance , ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4)
    {
        return (static_cast<C*>(instance.as_void)->*Function)(arg0, arg1, arg2, arg3, arg4);
    }

    template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4) const>
    static X_INLINE R ConstClassMethodStub(InstancePtr instance , ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4)
    {
        return (static_cast<const C*>(instance.as_const_void)->*Function)(arg0, arg1, arg2, arg3, arg4);
    }

public:
    template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4)>
    struct NonConstWrapper
    {
        NonConstWrapper(C* pInstance) :
            pInstance_(pInstance)
        {
        }

        C* pInstance_;
    };

    template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4) const>
    struct ConstWrapper
    {
        ConstWrapper(const C* pInstance) :
            pInstance_(pInstance)
        {
        }

        const C* pInstance_;
    };

    Delegate(void) :
        stub_()
    {
    }

    template<R (*Function)(ARG0, ARG1, ARG2, ARG3, ARG4)>
    void Bind(void)
    {
        stub_.instance.as_void = nullptr;
        stub_.pFunction = &FunctionStub<Function>;
    }

    template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4)>
    void Bind(NonConstWrapper<C, Function> wrapper)
    {
        stub_.instance.as_void = wrapper.pInstance_;
        stub_.pFunction = &ClassMethodStub<C, Function>;
    }

    template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4) const>
    void Bind(ConstWrapper<C, Function> wrapper)
    {
        stub_.instance.as_const_void = wrapper.pInstance_;
        stub_.pFunction = &ConstClassMethodStub<C, Function>;
    }

    R Invoke(ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4) const
    {
        X_ASSERT(stub_.pFunction != nullptr, "Cannot invoke unbound delegate. Call Bind() first.")();
        return stub_.pFunction(stub_.instance , arg0, arg1, arg2, arg3, arg4);
    }

    X_INLINE operator bool() const
    {
        return stub_.pFunction != nullptr;
    }

private:
    Stub stub_;
};


























template<typename R , typename ARG0, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5>
class Delegate<R(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5)>
{
    union InstancePtr
    {
        inline InstancePtr(void) :
            as_void(nullptr)
        {
        }

        inline bool operator==(const InstancePtr& other) const
        {
            return (as_void == other.as_void);
        }

        void* as_void;
        const void* as_const_void;
    };

    typedef traits::Function<R(InstancePtr , ARG0, ARG1, ARG2, ARG3, ARG4, ARG5)> InternalFunction;

    struct Stub
    {
        inline Stub(void) :
            instance(),
            pFunction(nullptr)
        {
        }

        inline bool operator==(const Stub& other) const
        {
            return ((instance == other.instance) && (pFunction == other.pFunction));
        }

        InstancePtr instance;
        typename InternalFunction::Pointer pFunction;
    };

    template<R (*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5)>
    static X_INLINE R FunctionStub(InstancePtr , ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5)
    {
        return (Function)(arg0, arg1, arg2, arg3, arg4, arg5);
    }

    template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5)>
    static X_INLINE R ClassMethodStub(InstancePtr instance , ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5)
    {
        return (static_cast<C*>(instance.as_void)->*Function)(arg0, arg1, arg2, arg3, arg4, arg5);
    }

    template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5) const>
    static X_INLINE R ConstClassMethodStub(InstancePtr instance , ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5)
    {
        return (static_cast<const C*>(instance.as_const_void)->*Function)(arg0, arg1, arg2, arg3, arg4, arg5);
    }

public:
    template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5)>
    struct NonConstWrapper
    {
        NonConstWrapper(C* pInstance) :
            pInstance_(pInstance)
        {
        }

        C* pInstance_;
    };

    template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5) const>
    struct ConstWrapper
    {
        ConstWrapper(const C* pInstance) :
            pInstance_(pInstance)
        {
        }

        const C* pInstance_;
    };

    Delegate(void) :
        stub_()
    {
    }

    template<R (*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5)>
    void Bind(void)
    {
        stub_.instance.as_void = nullptr;
        stub_.pFunction = &FunctionStub<Function>;
    }

    template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5)>
    void Bind(NonConstWrapper<C, Function> wrapper)
    {
        stub_.instance.as_void = wrapper.pInstance_;
        stub_.pFunction = &ClassMethodStub<C, Function>;
    }

    template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5) const>
    void Bind(ConstWrapper<C, Function> wrapper)
    {
        stub_.instance.as_const_void = wrapper.pInstance_;
        stub_.pFunction = &ConstClassMethodStub<C, Function>;
    }

    R Invoke(ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5) const
    {
        X_ASSERT(stub_.pFunction != nullptr, "Cannot invoke unbound delegate. Call Bind() first.")();
        return stub_.pFunction(stub_.instance , arg0, arg1, arg2, arg3, arg4, arg5);
    }

    X_INLINE operator bool() const
    {
        return stub_.pFunction != nullptr;
    }

private:
    Stub stub_;
};


























template<typename R , typename ARG0, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6>
class Delegate<R(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6)>
{
    union InstancePtr
    {
        inline InstancePtr(void) :
            as_void(nullptr)
        {
        }

        inline bool operator==(const InstancePtr& other) const
        {
            return (as_void == other.as_void);
        }

        void* as_void;
        const void* as_const_void;
    };

    typedef traits::Function<R(InstancePtr , ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6)> InternalFunction;

    struct Stub
    {
        inline Stub(void) :
            instance(),
            pFunction(nullptr)
        {
        }

        inline bool operator==(const Stub& other) const
        {
            return ((instance == other.instance) && (pFunction == other.pFunction));
        }

        InstancePtr instance;
        typename InternalFunction::Pointer pFunction;
    };

    template<R (*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6)>
    static X_INLINE R FunctionStub(InstancePtr , ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6)
    {
        return (Function)(arg0, arg1, arg2, arg3, arg4, arg5, arg6);
    }

    template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6)>
    static X_INLINE R ClassMethodStub(InstancePtr instance , ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6)
    {
        return (static_cast<C*>(instance.as_void)->*Function)(arg0, arg1, arg2, arg3, arg4, arg5, arg6);
    }

    template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6) const>
    static X_INLINE R ConstClassMethodStub(InstancePtr instance , ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6)
    {
        return (static_cast<const C*>(instance.as_const_void)->*Function)(arg0, arg1, arg2, arg3, arg4, arg5, arg6);
    }

public:
    template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6)>
    struct NonConstWrapper
    {
        NonConstWrapper(C* pInstance) :
            pInstance_(pInstance)
        {
        }

        C* pInstance_;
    };

    template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6) const>
    struct ConstWrapper
    {
        ConstWrapper(const C* pInstance) :
            pInstance_(pInstance)
        {
        }

        const C* pInstance_;
    };

    Delegate(void) :
        stub_()
    {
    }

    template<R (*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6)>
    void Bind(void)
    {
        stub_.instance.as_void = nullptr;
        stub_.pFunction = &FunctionStub<Function>;
    }

    template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6)>
    void Bind(NonConstWrapper<C, Function> wrapper)
    {
        stub_.instance.as_void = wrapper.pInstance_;
        stub_.pFunction = &ClassMethodStub<C, Function>;
    }

    template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6) const>
    void Bind(ConstWrapper<C, Function> wrapper)
    {
        stub_.instance.as_const_void = wrapper.pInstance_;
        stub_.pFunction = &ConstClassMethodStub<C, Function>;
    }

    R Invoke(ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6) const
    {
        X_ASSERT(stub_.pFunction != nullptr, "Cannot invoke unbound delegate. Call Bind() first.")();
        return stub_.pFunction(stub_.instance , arg0, arg1, arg2, arg3, arg4, arg5, arg6);
    }

    X_INLINE operator bool() const
    {
        return stub_.pFunction != nullptr;
    }

private:
    Stub stub_;
};


























template<typename R , typename ARG0, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6, typename ARG7>
class Delegate<R(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7)>
{
    union InstancePtr
    {
        inline InstancePtr(void) :
            as_void(nullptr)
        {
        }

        inline bool operator==(const InstancePtr& other) const
        {
            return (as_void == other.as_void);
        }

        void* as_void;
        const void* as_const_void;
    };

    typedef traits::Function<R(InstancePtr , ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7)> InternalFunction;

    struct Stub
    {
        inline Stub(void) :
            instance(),
            pFunction(nullptr)
        {
        }

        inline bool operator==(const Stub& other) const
        {
            return ((instance == other.instance) && (pFunction == other.pFunction));
        }

        InstancePtr instance;
        typename InternalFunction::Pointer pFunction;
    };

    template<R (*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7)>
    static X_INLINE R FunctionStub(InstancePtr , ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6, ARG7 arg7)
    {
        return (Function)(arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7);
    }

    template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7)>
    static X_INLINE R ClassMethodStub(InstancePtr instance , ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6, ARG7 arg7)
    {
        return (static_cast<C*>(instance.as_void)->*Function)(arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7);
    }

    template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7) const>
    static X_INLINE R ConstClassMethodStub(InstancePtr instance , ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6, ARG7 arg7)
    {
        return (static_cast<const C*>(instance.as_const_void)->*Function)(arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7);
    }

public:
    template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7)>
    struct NonConstWrapper
    {
        NonConstWrapper(C* pInstance) :
            pInstance_(pInstance)
        {
        }

        C* pInstance_;
    };

    template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7) const>
    struct ConstWrapper
    {
        ConstWrapper(const C* pInstance) :
            pInstance_(pInstance)
        {
        }

        const C* pInstance_;
    };

    Delegate(void) :
        stub_()
    {
    }

    template<R (*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7)>
    void Bind(void)
    {
        stub_.instance.as_void = nullptr;
        stub_.pFunction = &FunctionStub<Function>;
    }

    template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7)>
    void Bind(NonConstWrapper<C, Function> wrapper)
    {
        stub_.instance.as_void = wrapper.pInstance_;
        stub_.pFunction = &ClassMethodStub<C, Function>;
    }

    template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7) const>
    void Bind(ConstWrapper<C, Function> wrapper)
    {
        stub_.instance.as_const_void = wrapper.pInstance_;
        stub_.pFunction = &ConstClassMethodStub<C, Function>;
    }

    R Invoke(ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6, ARG7 arg7) const
    {
        X_ASSERT(stub_.pFunction != nullptr, "Cannot invoke unbound delegate. Call Bind() first.")();
        return stub_.pFunction(stub_.instance , arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7);
    }

    X_INLINE operator bool() const
    {
        return stub_.pFunction != nullptr;
    }

private:
    Stub stub_;
};













X_NAMESPACE_END
