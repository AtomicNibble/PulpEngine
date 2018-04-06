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
    /// Internal union that can hold a pointer to both non-const and const class instances.
    union InstancePtr
    {
        /// Default constructor, initializing to \c nullptr.
        inline InstancePtr(void) :
            as_void(nullptr)
        {
        }

        /// Comparison operator.
        inline bool operator==(const InstancePtr& other) const
        {
            return (as_void == other.as_void);
        }

        void* as_void;
        const void* as_const_void;
    };

    /// Internal type representing functions that are stored and used by the stub class.
    typedef traits::Function<R(InstancePtr X_PP_COMMA_IF(COUNT) ARG_TYPES)> InternalFunction;

    /// \brief Internal stub that holds both a class instance (if any) and a pointer to the function which is called
    /// when the delegate is invoked.
    struct Stub
    {
        /// Default constructor, initializing to \c nullptr.
        inline Stub(void) :
            instance(),
            function(nullptr)
        {
        }

        /// Comparison operator.
        inline bool operator==(const Stub& other) const
        {
            return ((instance == other.instance) && (function == other.function));
        }

        InstancePtr instance;
        typename InternalFunction::Pointer function;
    };

    /// Internal function used when binding delegates to free functions.
    template<R (*Function)(ARG_TYPES)>
    static X_INLINE R FunctionStub(InstancePtr X_PP_COMMA_IF(COUNT) ARGS)
    {
        // we don't need the instance pointer because we're dealing with free functions
        return (Function)(PASS_ARGS);
    }

    /// Internal function used when binding delegates to non-const member functions.
    template<class C, R (C::*Function)(ARG_TYPES)>
    static X_INLINE R ClassMethodStub(InstancePtr instance X_PP_COMMA_IF(COUNT) ARGS)
    {
        // cast the instance pointer back into the original class instance. this does not compromise type-safety
        // because we always know the concrete class type, given as a template argument.
        return (static_cast<C*>(instance.as_void)->*Function)(PASS_ARGS);
    }

    /// Internal function used when binding delegates to const member functions.
    template<class C, R (C::*Function)(ARG_TYPES) const>
    static X_INLINE R ConstClassMethodStub(InstancePtr instance X_PP_COMMA_IF(COUNT) ARGS)
    {
        // cast the instance pointer back into the original class instance. this does not compromise type-safety
        // because we always know the concrete class type, given as a template argument.
        return (static_cast<const C*>(instance.as_const_void)->*Function)(PASS_ARGS);
    }

public:
    /// Internal wrapper class used as a helper in Bind() overload resolution for non-const member functions.
    template<class C, R (C::*Function)(ARG_TYPES)>
    struct NonConstWrapper
    {
        /// Default constructor.
        NonConstWrapper(C* instance) :
            instance_(instance)
        {
        }

        C* instance_;
    };

    /// Internal wrapper class used as a helper in Bind() overload resolution for const member functions.
    template<class C, R (C::*Function)(ARG_TYPES) const>
    struct ConstWrapper
    {
        /// Default constructor.
        ConstWrapper(const C* instance) :
            instance_(instance)
        {
        }

        const C* instance_;
    };

    /// Default constructor.
    Delegate(void) :
        stub_()
    {
    }

    /// Binds a free function to the delegate.
    template<R (*Function)(ARG_TYPES)>
    void Bind(void)
    {
        stub_.instance.as_void = nullptr;
        stub_.function = &FunctionStub<Function>;
    }

    /// Binds a non-const class method to the delegate.
    template<class C, R (C::*Function)(ARG_TYPES)>
    void Bind(NonConstWrapper<C, Function> wrapper)
    {
        stub_.instance.as_void = wrapper.instance_;
        stub_.function = &ClassMethodStub<C, Function>;
    }

    /// Binds a const class method to the delegate.
    template<class C, R (C::*Function)(ARG_TYPES) const>
    void Bind(ConstWrapper<C, Function> wrapper)
    {
        stub_.instance.as_const_void = wrapper.instance_;
        stub_.function = &ConstClassMethodStub<C, Function>;
    }

    /// Invokes the delegate.
    R Invoke(ARGS) const
    {
        X_ASSERT(stub_.function != nullptr, "Cannot invoke unbound delegate. Call Bind() first.")();
        return stub_.function(stub_.instance X_PP_COMMA_IF(COUNT) PASS_ARGS);
    }

    X_INLINE operator bool() const
    {
        return stub_.function != nullptr;
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
