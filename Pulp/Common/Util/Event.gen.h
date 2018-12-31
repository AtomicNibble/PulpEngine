#define ARG_TYPENAMES_IMPL(n) typename ARG##n
#define ARG_TYPENAMES X_PP_LIST(COUNT, ARG_TYPENAMES_IMPL)

#define ARG_TYPES_IMPL(n) ARG##n
#define ARG_TYPES X_PP_LIST(COUNT, ARG_TYPES_IMPL)

#define ARGS_IMPL(n) ARG##n arg##n
#define ARGS X_PP_LIST(COUNT, ARGS_IMPL)

#define PASS_ARGS_IMPL(n) arg##n
#define PASS_ARGS X_PP_LIST(COUNT, PASS_ARGS_IMPL)

template<typename R X_PP_COMMA_IF(COUNT) ARG_TYPENAMES>
class Event<R(ARG_TYPES)>
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
    class Sink
    {
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
            ConstWrapper(const C* instance) :
                pInstance_(instance)
            {
            }

            const C* pInstance_;
        };

        Sink(MemoryArenaBase* arena) :
            listeners_(arena)
        {
        }

        Sink(MemoryArenaBase* arena, size_t numListeners) :
            Sink(arena)
        {
            listeners_.reserve(numListeners);
        }

        Sink(Sink&& oth) = default;
        Sink(const Sink& oth) = default;
        Sink& operator=(Sink&& oth) = default;
        Sink& operator=(const Sink& oth) = default;

        template<R (*Function)(ARG_TYPES)>
        void AddListener(void)
        {
            Stub stub;
            stub.instance.as_void = nullptr;
            stub.pFunction = &FunctionStub<Function>;

            listeners_.append(stub);
        }

        template<class C, R (C::*Function)(ARG_TYPES)>
        void AddListener(NonConstWrapper<C, Function> wrapper)
        {
            Stub stub;
            stub.instance.as_void = wrapper.pInstance_;
            stub.pFunction = &ClassMethodStub<C, Function>;

            listeners_.append(stub);
        }

        template<class C, R (C::*Function)(ARG_TYPES) const>
        void AddListener(ConstWrapper<C, Function> wrapper)
        {
            Stub stub;
            stub.instance.as_const_void = wrapper.pInstance_;
            stub.pFunction = &ConstClassMethodStub<C, Function>;

            listeners_.append(stub);
        }

        template<R (*Function)(ARG_TYPES)>
        void RemoveListener(void)
        {
            Stub stub;
            stub.instance.as_void = nullptr;
            stub.pFunction = &FunctionStub<Function>;

            listeners_.remove(stub);
        }

        template<class C, R (C::*Function)(ARG_TYPES)>
        void RemoveListener(NonConstWrapper<C, Function> wrapper)
        {
            Stub stub;
            stub.instance.as_void = wrapper.pInstance_;
            stub.pFunction = &ClassMethodStub<C, Function>;

            listeners_.remove(stub);
        }

        template<class C, R (C::*Function)(ARG_TYPES) const>
        void RemoveListener(ConstWrapper<C, Function> wrapper)
        {
            Stub stub;
            stub.instance.as_const_void = wrapper.pInstance_;
            stub.pFunction = &ConstClassMethodStub<C, Function>;

            listeners_.remove(stub);
        }

        inline size_t GetListenerCount(void) const
        {
            return listeners_.size();
        }

        inline const Stub& GetListener(size_t i) const
        {
            return listeners_[i];
        }

    private:
        core::Array<Stub> listeners_;
    };

    Event(void) :
        pSink_(nullptr)
    {
    }
    
    inline void Bind(Sink* pSink)
    {
        pSink_ = pSink;
    }

    void Signal(ARGS) const
    {
        X_ASSERT(pSink_ != nullptr, "Cannot signal unbound event. Call Bind() first.")();

        for (size_t i = 0; i < pSink_->GetListenerCount(); ++i) {
            const Stub& stub = pSink_->GetListener(i);
            stub.pFunction(stub.instance X_PP_COMMA_IF(COUNT) PASS_ARGS);
        }
    }

    inline bool IsBound(void) const
    {
        return (pSink_ != nullptr);
    }

private:
    X_NO_COPY(Event);
    X_NO_ASSIGN(Event);

    Sink* pSink_;
};

#undef ARG_TYPENAMES_IMPL
#undef ARG_TYPENAMES
#undef ARG_TYPES_IMPL
#undef ARG_TYPES
#undef ARGS_IMPL
#undef ARGS
#undef PASS_ARGS_IMPL
#undef PASS_ARGS
