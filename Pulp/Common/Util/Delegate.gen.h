#define ARG_TYPENAMES_IMPL(n) typename ARG##n
#define ARG_TYPENAMES X_PP_LIST(COUNT, ARG_TYPENAMES_IMPL)

#define ARG_TYPES_IMPL(n) ARG##n
#define ARG_TYPES X_PP_LIST(COUNT, ARG_TYPES_IMPL)

#define ARGS_IMPL(n) ARG##n arg##n
#define ARGS X_PP_LIST(COUNT, ARGS_IMPL)

#define PASS_ARGS_IMPL(n) arg##n
#define PASS_ARGS X_PP_LIST(COUNT, PASS_ARGS_IMPL)

template<typename R X_PP_COMMA_IF(COUNT) ARG_TYPENAMES>
class Delegate<R(ARG_TYPES)>
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

    typedef traits::Function<R(InstancePtr X_PP_COMMA_IF(COUNT) ARG_TYPES)> InternalFunction;

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

    template<R (*Function)(ARG_TYPES)>
    static X_INLINE R FunctionStub(InstancePtr X_PP_COMMA_IF(COUNT) ARGS)
    {
        return (Function)(PASS_ARGS);
    }

    template<class C, R (C::*Function)(ARG_TYPES)>
    static X_INLINE R ClassMethodStub(InstancePtr instance X_PP_COMMA_IF(COUNT) ARGS)
    {
        return (static_cast<C*>(instance.as_void)->*Function)(PASS_ARGS);
    }

    template<class C, R (C::*Function)(ARG_TYPES) const>
    static X_INLINE R ConstClassMethodStub(InstancePtr instance X_PP_COMMA_IF(COUNT) ARGS)
    {
        return (static_cast<const C*>(instance.as_const_void)->*Function)(PASS_ARGS);
    }

public:
    template<class C, R (C::*Function)(ARG_TYPES)>
    struct NonConstWrapper
    {
        NonConstWrapper(C* pInstance) :
            pInstance_(pInstance)
        {
        }

        C* pInstance_;
    };

    template<class C, R (C::*Function)(ARG_TYPES) const>
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

    template<R (*Function)(ARG_TYPES)>
    void Bind(void)
    {
        stub_.instance.as_void = nullptr;
        stub_.pFunction = &FunctionStub<Function>;
    }

    template<class C, R (C::*Function)(ARG_TYPES)>
    void Bind(NonConstWrapper<C, Function> wrapper)
    {
        stub_.instance.as_void = wrapper.pInstance_;
        stub_.pFunction = &ClassMethodStub<C, Function>;
    }

    template<class C, R (C::*Function)(ARG_TYPES) const>
    void Bind(ConstWrapper<C, Function> wrapper)
    {
        stub_.instance.as_const_void = wrapper.pInstance_;
        stub_.pFunction = &ConstClassMethodStub<C, Function>;
    }

    R Invoke(ARGS) const
    {
        X_ASSERT(stub_.pFunction != nullptr, "Cannot invoke unbound delegate. Call Bind() first.")();
        return stub_.pFunction(stub_.instance X_PP_COMMA_IF(COUNT) PASS_ARGS);
    }

    X_INLINE operator bool() const
    {
        return stub_.pFunction != nullptr;
    }

private:
    Stub stub_;
};

#undef ARG_TYPENAMES_IMPL
#undef ARG_TYPENAMES
#undef ARG_TYPES_IMPL
#undef ARG_TYPES
#undef ARGS_IMPL
#undef ARGS
#undef PASS_ARGS_IMPL
#undef PASS_ARGS
