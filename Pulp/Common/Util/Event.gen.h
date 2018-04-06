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
    /// when the event is signaled.
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

    /// Internal function used when binding sinks to free functions.
    template<R (*Function)(ARG_TYPES)>
    static X_INLINE R FunctionStub(InstancePtr X_PP_COMMA_IF(COUNT) ARGS)
    {
        // we don't need the instance pointer because we're dealing with free functions
        return (Function)(PASS_ARGS);
    }

    /// Internal function used when binding sinks to non-const member functions.
    template<class C, R (C::*Function)(ARG_TYPES)>
    static X_INLINE R ClassMethodStub(InstancePtr instance X_PP_COMMA_IF(COUNT) ARGS)
    {
        // cast the instance pointer back into the original class instance. this does not compromise type-safety
        // because we always know the concrete class type, given as a template argument.
        return (static_cast<C*>(instance.as_void)->*Function)(PASS_ARGS);
    }

    /// Internal function used when binding sinks to const member functions.
    template<class C, R (C::*Function)(ARG_TYPES) const>
    static X_INLINE R ConstClassMethodStub(InstancePtr instance X_PP_COMMA_IF(COUNT) ARGS)
    {
        // cast the instance pointer back into the original class instance. this does not compromise type-safety
        // because we always know the concrete class type, given as a template argument.
        return (static_cast<const C*>(instance.as_const_void)->*Function)(PASS_ARGS);
    }

public:
    /// \brief A class responsible for adding/removing listeners which are to be signaled by an Event.
    class Sink
    {
    public:
        /// Internal wrapper class used as a helper in AddListener()/RemoveListener() overload resolution for non-const member functions.
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

        /// Internal wrapper class used as a helper in AddListener()/RemoveListener() overload resolution for const member functions.
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

        /// \brief Constructs a sink, allocating memory to store the given number of listeners in an array.
        /// \sa Array
        Sink(MemoryArenaBase* arena, size_t numListeners) :
            listeners_(arena)
        {
            listeners_.reserve(numListeners);
        }

        /// Adds a free function as listener.
        template<R (*Function)(ARG_TYPES)>
        void AddListener(void)
        {
            Stub stub;
            stub.instance.as_void = nullptr;
            stub.function = &FunctionStub<Function>;

            listeners_.append(stub);
        }

        /// Adds a non-const class method as listener.
        template<class C, R (C::*Function)(ARG_TYPES)>
        void AddListener(NonConstWrapper<C, Function> wrapper)
        {
            Stub stub;
            stub.instance.as_void = wrapper.instance_;
            stub.function = &ClassMethodStub<C, Function>;

            listeners_.append(stub);
        }

        /// Adds a const class method as listener.
        template<class C, R (C::*Function)(ARG_TYPES) const>
        void AddListener(ConstWrapper<C, Function> wrapper)
        {
            Stub stub;
            stub.instance.as_const_void = wrapper.instance_;
            stub.function = &ConstClassMethodStub<C, Function>;

            listeners_.append(stub);
        }

        /// \brief Removes a free function as listener.
        /// \remark The order of listeners inside the sink is not preserved.
        template<R (*Function)(ARG_TYPES)>
        void RemoveListener(void)
        {
            Stub stub;
            stub.instance.as_void = nullptr;
            stub.function = &FunctionStub<Function>;

            listeners_.remove(stub);
        }

        /// \brief Removes a non-const class method as listener.
        /// \remark The order of listeners inside the sink is not preserved.
        template<class C, R (C::*Function)(ARG_TYPES)>
        void RemoveListener(NonConstWrapper<C, Function> wrapper)
        {
            Stub stub;
            stub.instance.as_void = wrapper.instance_;
            stub.function = &ClassMethodStub<C, Function>;

            listeners_.remove(stub);
        }

        /// \brief Removes a const class method as listener.
        /// \remark The order of listeners inside the sink is not preserved.
        template<class C, R (C::*Function)(ARG_TYPES) const>
        void RemoveListener(ConstWrapper<C, Function> wrapper)
        {
            Stub stub;
            stub.instance.as_const_void = wrapper.instance_;
            stub.function = &ConstClassMethodStub<C, Function>;

            listeners_.remove(stub);
        }

        /// Returns the number of listeners.
        inline size_t GetListenerCount(void) const
        {
            return listeners_.size();
        }

        /// Returns the i-th listener.
        inline const Stub& GetListener(size_t i) const
        {
            return listeners_[i];
        }

    private:
        X_NO_COPY(Sink);
        X_NO_ASSIGN(Sink);

        core::Array<Stub> listeners_;
    };

    /// Default constructor.
    Event(void) :
        sink_(nullptr)
    {
    }

    /// Binds a sink to the event.
    inline void Bind(Sink* sink)
    {
        sink_ = sink;
    }

    /// Signals the sink.
    void Signal(ARGS) const
    {
        X_ASSERT(sink_ != nullptr, "Cannot signal unbound event. Call Bind() first.")
        ();

        for (size_t i = 0; i < sink_->GetListenerCount(); ++i) {
            const Stub& stub = sink_->GetListener(i);
            stub.function(stub.instance X_PP_COMMA_IF(COUNT) PASS_ARGS);
        }
    }

    /// Returns whether a sink is bound to the event.
    inline bool IsBound(void) const
    {
        return (sink_ != nullptr);
    }

private:
    X_NO_COPY(Event);
    X_NO_ASSIGN(Event);

    Sink* sink_;
};

#undef ARG_TYPENAMES_IMPL
#undef ARG_TYPENAMES
#undef ARG_TYPES_IMPL
#undef ARG_TYPES
#undef ARGS_IMPL
#undef ARGS
#undef PASS_ARGS_IMPL
#undef PASS_ARGS
