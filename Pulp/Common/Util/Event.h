
#pragma once
#pragma once



#pragma once














#pragma once












#pragma once






































#pragma once




































#pragma once






























X_NAMESPACE_BEGIN(core)

template <typename T>
class Event {};















template<typename R  >
class Event<R()>
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
    class Sink
    {
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

        Sink(MemoryArenaBase* arena, size_t reserve) :
            Sink(arena)
        {
            listeners_.reserve(reserve);
        }

        Sink(Sink&& oth) = default;
        Sink(const Sink& oth) = default;
        Sink& operator=(Sink&& oth) = default;
        Sink& operator=(const Sink& oth) = default;

        template<R (*Function)()>
        void AddListener(void)
        {
            Stub stub;
            stub.instance.as_void = nullptr;
            stub.pFunction = &FunctionStub<Function>;

            listeners_.append(stub);
        }

        template<class C, R (C::*Function)()>
        void AddListener(NonConstWrapper<C, Function> wrapper)
        {
            Stub stub;
            stub.instance.as_void = wrapper.pInstance_;
            stub.pFunction = &ClassMethodStub<C, Function>;

            listeners_.append(stub);
        }

        template<class C, R (C::*Function)() const>
        void AddListener(ConstWrapper<C, Function> wrapper)
        {
            Stub stub;
            stub.instance.as_const_void = wrapper.pInstance_;
            stub.pFunction = &ConstClassMethodStub<C, Function>;

            listeners_.append(stub);
        }

        template<R (*Function)()>
        void RemoveListener(void)
        {
            Stub stub;
            stub.instance.as_void = nullptr;
            stub.pFunction = &FunctionStub<Function>;

            listeners_.remove(stub);
        }

        template<class C, R (C::*Function)()>
        void RemoveListener(NonConstWrapper<C, Function> wrapper)
        {
            Stub stub;
            stub.instance.as_void = wrapper.pInstance_;
            stub.pFunction = &ClassMethodStub<C, Function>;

            listeners_.remove(stub);
        }

        template<class C, R (C::*Function)() const>
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

    void Signal() const
    {
        X_ASSERT(pSink_ != nullptr, "Cannot signal unbound event. Call Bind() first.")();

        for (size_t i = 0; i < pSink_->GetListenerCount(); ++i) {
            const Stub& stub = pSink_->GetListener(i);
            stub.pFunction(stub.instance  );
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


























template<typename R , typename ARG0>
class Event<R(ARG0)>
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
    class Sink
    {
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

        Sink(MemoryArenaBase* arena, size_t reserve) :
            Sink(arena)
        {
            listeners_.reserve(reserve);
        }

        Sink(Sink&& oth) = default;
        Sink(const Sink& oth) = default;
        Sink& operator=(Sink&& oth) = default;
        Sink& operator=(const Sink& oth) = default;

        template<R (*Function)(ARG0)>
        void AddListener(void)
        {
            Stub stub;
            stub.instance.as_void = nullptr;
            stub.pFunction = &FunctionStub<Function>;

            listeners_.append(stub);
        }

        template<class C, R (C::*Function)(ARG0)>
        void AddListener(NonConstWrapper<C, Function> wrapper)
        {
            Stub stub;
            stub.instance.as_void = wrapper.pInstance_;
            stub.pFunction = &ClassMethodStub<C, Function>;

            listeners_.append(stub);
        }

        template<class C, R (C::*Function)(ARG0) const>
        void AddListener(ConstWrapper<C, Function> wrapper)
        {
            Stub stub;
            stub.instance.as_const_void = wrapper.pInstance_;
            stub.pFunction = &ConstClassMethodStub<C, Function>;

            listeners_.append(stub);
        }

        template<R (*Function)(ARG0)>
        void RemoveListener(void)
        {
            Stub stub;
            stub.instance.as_void = nullptr;
            stub.pFunction = &FunctionStub<Function>;

            listeners_.remove(stub);
        }

        template<class C, R (C::*Function)(ARG0)>
        void RemoveListener(NonConstWrapper<C, Function> wrapper)
        {
            Stub stub;
            stub.instance.as_void = wrapper.pInstance_;
            stub.pFunction = &ClassMethodStub<C, Function>;

            listeners_.remove(stub);
        }

        template<class C, R (C::*Function)(ARG0) const>
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

    void Signal(ARG0 arg0) const
    {
        X_ASSERT(pSink_ != nullptr, "Cannot signal unbound event. Call Bind() first.")();

        for (size_t i = 0; i < pSink_->GetListenerCount(); ++i) {
            const Stub& stub = pSink_->GetListener(i);
            stub.pFunction(stub.instance , arg0);
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


























template<typename R , typename ARG0, typename ARG1>
class Event<R(ARG0, ARG1)>
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
    class Sink
    {
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

        Sink(MemoryArenaBase* arena, size_t reserve) :
            Sink(arena)
        {
            listeners_.reserve(reserve);
        }

        Sink(Sink&& oth) = default;
        Sink(const Sink& oth) = default;
        Sink& operator=(Sink&& oth) = default;
        Sink& operator=(const Sink& oth) = default;

        template<R (*Function)(ARG0, ARG1)>
        void AddListener(void)
        {
            Stub stub;
            stub.instance.as_void = nullptr;
            stub.pFunction = &FunctionStub<Function>;

            listeners_.append(stub);
        }

        template<class C, R (C::*Function)(ARG0, ARG1)>
        void AddListener(NonConstWrapper<C, Function> wrapper)
        {
            Stub stub;
            stub.instance.as_void = wrapper.pInstance_;
            stub.pFunction = &ClassMethodStub<C, Function>;

            listeners_.append(stub);
        }

        template<class C, R (C::*Function)(ARG0, ARG1) const>
        void AddListener(ConstWrapper<C, Function> wrapper)
        {
            Stub stub;
            stub.instance.as_const_void = wrapper.pInstance_;
            stub.pFunction = &ConstClassMethodStub<C, Function>;

            listeners_.append(stub);
        }

        template<R (*Function)(ARG0, ARG1)>
        void RemoveListener(void)
        {
            Stub stub;
            stub.instance.as_void = nullptr;
            stub.pFunction = &FunctionStub<Function>;

            listeners_.remove(stub);
        }

        template<class C, R (C::*Function)(ARG0, ARG1)>
        void RemoveListener(NonConstWrapper<C, Function> wrapper)
        {
            Stub stub;
            stub.instance.as_void = wrapper.pInstance_;
            stub.pFunction = &ClassMethodStub<C, Function>;

            listeners_.remove(stub);
        }

        template<class C, R (C::*Function)(ARG0, ARG1) const>
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

    void Signal(ARG0 arg0, ARG1 arg1) const
    {
        X_ASSERT(pSink_ != nullptr, "Cannot signal unbound event. Call Bind() first.")();

        for (size_t i = 0; i < pSink_->GetListenerCount(); ++i) {
            const Stub& stub = pSink_->GetListener(i);
            stub.pFunction(stub.instance , arg0, arg1);
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


























template<typename R , typename ARG0, typename ARG1, typename ARG2>
class Event<R(ARG0, ARG1, ARG2)>
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
    class Sink
    {
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

        Sink(MemoryArenaBase* arena, size_t reserve) :
            Sink(arena)
        {
            listeners_.reserve(reserve);
        }

        Sink(Sink&& oth) = default;
        Sink(const Sink& oth) = default;
        Sink& operator=(Sink&& oth) = default;
        Sink& operator=(const Sink& oth) = default;

        template<R (*Function)(ARG0, ARG1, ARG2)>
        void AddListener(void)
        {
            Stub stub;
            stub.instance.as_void = nullptr;
            stub.pFunction = &FunctionStub<Function>;

            listeners_.append(stub);
        }

        template<class C, R (C::*Function)(ARG0, ARG1, ARG2)>
        void AddListener(NonConstWrapper<C, Function> wrapper)
        {
            Stub stub;
            stub.instance.as_void = wrapper.pInstance_;
            stub.pFunction = &ClassMethodStub<C, Function>;

            listeners_.append(stub);
        }

        template<class C, R (C::*Function)(ARG0, ARG1, ARG2) const>
        void AddListener(ConstWrapper<C, Function> wrapper)
        {
            Stub stub;
            stub.instance.as_const_void = wrapper.pInstance_;
            stub.pFunction = &ConstClassMethodStub<C, Function>;

            listeners_.append(stub);
        }

        template<R (*Function)(ARG0, ARG1, ARG2)>
        void RemoveListener(void)
        {
            Stub stub;
            stub.instance.as_void = nullptr;
            stub.pFunction = &FunctionStub<Function>;

            listeners_.remove(stub);
        }

        template<class C, R (C::*Function)(ARG0, ARG1, ARG2)>
        void RemoveListener(NonConstWrapper<C, Function> wrapper)
        {
            Stub stub;
            stub.instance.as_void = wrapper.pInstance_;
            stub.pFunction = &ClassMethodStub<C, Function>;

            listeners_.remove(stub);
        }

        template<class C, R (C::*Function)(ARG0, ARG1, ARG2) const>
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

    void Signal(ARG0 arg0, ARG1 arg1, ARG2 arg2) const
    {
        X_ASSERT(pSink_ != nullptr, "Cannot signal unbound event. Call Bind() first.")();

        for (size_t i = 0; i < pSink_->GetListenerCount(); ++i) {
            const Stub& stub = pSink_->GetListener(i);
            stub.pFunction(stub.instance , arg0, arg1, arg2);
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


























template<typename R , typename ARG0, typename ARG1, typename ARG2, typename ARG3>
class Event<R(ARG0, ARG1, ARG2, ARG3)>
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
    class Sink
    {
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

        Sink(MemoryArenaBase* arena, size_t reserve) :
            Sink(arena)
        {
            listeners_.reserve(reserve);
        }

        Sink(Sink&& oth) = default;
        Sink(const Sink& oth) = default;
        Sink& operator=(Sink&& oth) = default;
        Sink& operator=(const Sink& oth) = default;

        template<R (*Function)(ARG0, ARG1, ARG2, ARG3)>
        void AddListener(void)
        {
            Stub stub;
            stub.instance.as_void = nullptr;
            stub.pFunction = &FunctionStub<Function>;

            listeners_.append(stub);
        }

        template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3)>
        void AddListener(NonConstWrapper<C, Function> wrapper)
        {
            Stub stub;
            stub.instance.as_void = wrapper.pInstance_;
            stub.pFunction = &ClassMethodStub<C, Function>;

            listeners_.append(stub);
        }

        template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3) const>
        void AddListener(ConstWrapper<C, Function> wrapper)
        {
            Stub stub;
            stub.instance.as_const_void = wrapper.pInstance_;
            stub.pFunction = &ConstClassMethodStub<C, Function>;

            listeners_.append(stub);
        }

        template<R (*Function)(ARG0, ARG1, ARG2, ARG3)>
        void RemoveListener(void)
        {
            Stub stub;
            stub.instance.as_void = nullptr;
            stub.pFunction = &FunctionStub<Function>;

            listeners_.remove(stub);
        }

        template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3)>
        void RemoveListener(NonConstWrapper<C, Function> wrapper)
        {
            Stub stub;
            stub.instance.as_void = wrapper.pInstance_;
            stub.pFunction = &ClassMethodStub<C, Function>;

            listeners_.remove(stub);
        }

        template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3) const>
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

    void Signal(ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3) const
    {
        X_ASSERT(pSink_ != nullptr, "Cannot signal unbound event. Call Bind() first.")();

        for (size_t i = 0; i < pSink_->GetListenerCount(); ++i) {
            const Stub& stub = pSink_->GetListener(i);
            stub.pFunction(stub.instance , arg0, arg1, arg2, arg3);
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


























template<typename R , typename ARG0, typename ARG1, typename ARG2, typename ARG3, typename ARG4>
class Event<R(ARG0, ARG1, ARG2, ARG3, ARG4)>
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
    class Sink
    {
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

        Sink(MemoryArenaBase* arena, size_t reserve) :
            Sink(arena)
        {
            listeners_.reserve(reserve);
        }

        Sink(Sink&& oth) = default;
        Sink(const Sink& oth) = default;
        Sink& operator=(Sink&& oth) = default;
        Sink& operator=(const Sink& oth) = default;

        template<R (*Function)(ARG0, ARG1, ARG2, ARG3, ARG4)>
        void AddListener(void)
        {
            Stub stub;
            stub.instance.as_void = nullptr;
            stub.pFunction = &FunctionStub<Function>;

            listeners_.append(stub);
        }

        template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4)>
        void AddListener(NonConstWrapper<C, Function> wrapper)
        {
            Stub stub;
            stub.instance.as_void = wrapper.pInstance_;
            stub.pFunction = &ClassMethodStub<C, Function>;

            listeners_.append(stub);
        }

        template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4) const>
        void AddListener(ConstWrapper<C, Function> wrapper)
        {
            Stub stub;
            stub.instance.as_const_void = wrapper.pInstance_;
            stub.pFunction = &ConstClassMethodStub<C, Function>;

            listeners_.append(stub);
        }

        template<R (*Function)(ARG0, ARG1, ARG2, ARG3, ARG4)>
        void RemoveListener(void)
        {
            Stub stub;
            stub.instance.as_void = nullptr;
            stub.pFunction = &FunctionStub<Function>;

            listeners_.remove(stub);
        }

        template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4)>
        void RemoveListener(NonConstWrapper<C, Function> wrapper)
        {
            Stub stub;
            stub.instance.as_void = wrapper.pInstance_;
            stub.pFunction = &ClassMethodStub<C, Function>;

            listeners_.remove(stub);
        }

        template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4) const>
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

    void Signal(ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4) const
    {
        X_ASSERT(pSink_ != nullptr, "Cannot signal unbound event. Call Bind() first.")();

        for (size_t i = 0; i < pSink_->GetListenerCount(); ++i) {
            const Stub& stub = pSink_->GetListener(i);
            stub.pFunction(stub.instance , arg0, arg1, arg2, arg3, arg4);
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


























template<typename R , typename ARG0, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5>
class Event<R(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5)>
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
    class Sink
    {
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

        Sink(MemoryArenaBase* arena, size_t reserve) :
            Sink(arena)
        {
            listeners_.reserve(reserve);
        }

        Sink(Sink&& oth) = default;
        Sink(const Sink& oth) = default;
        Sink& operator=(Sink&& oth) = default;
        Sink& operator=(const Sink& oth) = default;

        template<R (*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5)>
        void AddListener(void)
        {
            Stub stub;
            stub.instance.as_void = nullptr;
            stub.pFunction = &FunctionStub<Function>;

            listeners_.append(stub);
        }

        template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5)>
        void AddListener(NonConstWrapper<C, Function> wrapper)
        {
            Stub stub;
            stub.instance.as_void = wrapper.pInstance_;
            stub.pFunction = &ClassMethodStub<C, Function>;

            listeners_.append(stub);
        }

        template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5) const>
        void AddListener(ConstWrapper<C, Function> wrapper)
        {
            Stub stub;
            stub.instance.as_const_void = wrapper.pInstance_;
            stub.pFunction = &ConstClassMethodStub<C, Function>;

            listeners_.append(stub);
        }

        template<R (*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5)>
        void RemoveListener(void)
        {
            Stub stub;
            stub.instance.as_void = nullptr;
            stub.pFunction = &FunctionStub<Function>;

            listeners_.remove(stub);
        }

        template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5)>
        void RemoveListener(NonConstWrapper<C, Function> wrapper)
        {
            Stub stub;
            stub.instance.as_void = wrapper.pInstance_;
            stub.pFunction = &ClassMethodStub<C, Function>;

            listeners_.remove(stub);
        }

        template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5) const>
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

    void Signal(ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5) const
    {
        X_ASSERT(pSink_ != nullptr, "Cannot signal unbound event. Call Bind() first.")();

        for (size_t i = 0; i < pSink_->GetListenerCount(); ++i) {
            const Stub& stub = pSink_->GetListener(i);
            stub.pFunction(stub.instance , arg0, arg1, arg2, arg3, arg4, arg5);
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


























template<typename R , typename ARG0, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6>
class Event<R(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6)>
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
    class Sink
    {
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

        Sink(MemoryArenaBase* arena, size_t reserve) :
            Sink(arena)
        {
            listeners_.reserve(reserve);
        }

        Sink(Sink&& oth) = default;
        Sink(const Sink& oth) = default;
        Sink& operator=(Sink&& oth) = default;
        Sink& operator=(const Sink& oth) = default;

        template<R (*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6)>
        void AddListener(void)
        {
            Stub stub;
            stub.instance.as_void = nullptr;
            stub.pFunction = &FunctionStub<Function>;

            listeners_.append(stub);
        }

        template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6)>
        void AddListener(NonConstWrapper<C, Function> wrapper)
        {
            Stub stub;
            stub.instance.as_void = wrapper.pInstance_;
            stub.pFunction = &ClassMethodStub<C, Function>;

            listeners_.append(stub);
        }

        template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6) const>
        void AddListener(ConstWrapper<C, Function> wrapper)
        {
            Stub stub;
            stub.instance.as_const_void = wrapper.pInstance_;
            stub.pFunction = &ConstClassMethodStub<C, Function>;

            listeners_.append(stub);
        }

        template<R (*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6)>
        void RemoveListener(void)
        {
            Stub stub;
            stub.instance.as_void = nullptr;
            stub.pFunction = &FunctionStub<Function>;

            listeners_.remove(stub);
        }

        template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6)>
        void RemoveListener(NonConstWrapper<C, Function> wrapper)
        {
            Stub stub;
            stub.instance.as_void = wrapper.pInstance_;
            stub.pFunction = &ClassMethodStub<C, Function>;

            listeners_.remove(stub);
        }

        template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6) const>
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

    void Signal(ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6) const
    {
        X_ASSERT(pSink_ != nullptr, "Cannot signal unbound event. Call Bind() first.")();

        for (size_t i = 0; i < pSink_->GetListenerCount(); ++i) {
            const Stub& stub = pSink_->GetListener(i);
            stub.pFunction(stub.instance , arg0, arg1, arg2, arg3, arg4, arg5, arg6);
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


























template<typename R , typename ARG0, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6, typename ARG7>
class Event<R(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7)>
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
    class Sink
    {
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

        Sink(MemoryArenaBase* arena, size_t reserve) :
            Sink(arena)
        {
            listeners_.reserve(reserve);
        }

        Sink(Sink&& oth) = default;
        Sink(const Sink& oth) = default;
        Sink& operator=(Sink&& oth) = default;
        Sink& operator=(const Sink& oth) = default;

        template<R (*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7)>
        void AddListener(void)
        {
            Stub stub;
            stub.instance.as_void = nullptr;
            stub.pFunction = &FunctionStub<Function>;

            listeners_.append(stub);
        }

        template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7)>
        void AddListener(NonConstWrapper<C, Function> wrapper)
        {
            Stub stub;
            stub.instance.as_void = wrapper.pInstance_;
            stub.pFunction = &ClassMethodStub<C, Function>;

            listeners_.append(stub);
        }

        template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7) const>
        void AddListener(ConstWrapper<C, Function> wrapper)
        {
            Stub stub;
            stub.instance.as_const_void = wrapper.pInstance_;
            stub.pFunction = &ConstClassMethodStub<C, Function>;

            listeners_.append(stub);
        }

        template<R (*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7)>
        void RemoveListener(void)
        {
            Stub stub;
            stub.instance.as_void = nullptr;
            stub.pFunction = &FunctionStub<Function>;

            listeners_.remove(stub);
        }

        template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7)>
        void RemoveListener(NonConstWrapper<C, Function> wrapper)
        {
            Stub stub;
            stub.instance.as_void = wrapper.pInstance_;
            stub.pFunction = &ClassMethodStub<C, Function>;

            listeners_.remove(stub);
        }

        template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7) const>
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

    void Signal(ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6, ARG7 arg7) const
    {
        X_ASSERT(pSink_ != nullptr, "Cannot signal unbound event. Call Bind() first.")();

        for (size_t i = 0; i < pSink_->GetListenerCount(); ++i) {
            const Stub& stub = pSink_->GetListener(i);
            stub.pFunction(stub.instance , arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7);
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













X_NAMESPACE_END
