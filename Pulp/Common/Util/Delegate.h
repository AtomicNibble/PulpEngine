
#pragma once



#pragma once




/// \def X_PP_COMMA
/// \brief Internal macro used by \ref X_PP_COMMA_IF.
/// \sa X_PP_COMMA_IF


/// \def X_PP_COMMA_EMPTY
/// \brief Internal macro used by \ref X_PP_COMMA_IF.
/// \sa X_PP_COMMA_IF


/// \def X_PP_COMMA_IF
/// \ingroup Preprocessor
/// \brief Outputs a comma if the given condition is true (!= 0), otherwise outputs nothing.
/// \details This macro can be used to output a comma for a comma-separated list of arguments based on some condition,
/// such as e.g. the number of arguments for any declaration. The following example shows a common usage:
/// \code
///   // assuming ARG_TYPENAMES is a macro, it can either be a comma-separated list of arguments, or empty
///   template <typename R, ARG_TYPENAMES>
///
///   // if ARG_TYPENAMES contains a comma-separated list of arguments, the macro expands to the following
///   template <typename R, typename T1, typename T2, ...>
///
///   // if ARG_TYPENAMES is empty, the extra comma would cause a syntax error
///   template <typename R, >
///
///   // using X_PP_COMMA_IF corrects this problem without having to special-case each such macro.
///   // this will only output a comma if COUNT != 0, solving the problem.
///   template <typename R X_PP_COMMA_IF(COUNT) ARG_TYPENAMES>
/// \endcode





#pragma once




/// \def X_PP_IF_0
/// \brief Internal macro used by \ref X_PP_IF.
/// \sa X_PP_IF


/// \def X_PP_IF_1
/// \brief Internal macro used by \ref X_PP_IF.
/// \sa X_PP_IF


/// \def X_PP_IF
/// \ingroup Preprocessor
/// \brief Chooses one of two given values based on the value of a condition.
/// \details The condition can either be a boolean value, or an integer.
/// \code
///   // boolean values
///   X_PP_IF(true, 10, 20);			// outputs 10
///   X_PP_IF(false, 10, 20);			// outputs 20
///
///   // integer values
///   X_PP_IF(0, 10, 20);				// outputs 20
///   X_PP_IF(1, 10, 20);				// outputs 10
///   X_PP_IF(10, 10, 20);				// outputs 10
/// \endcode






#pragma once




/// \def X_PP_JOIN_HELPER_HELPER
/// \brief Internal macro used by \ref X_PP_JOIN.
/// \sa X_PP_JOIN


/// \def X_PP_JOIN_HELPER
/// \brief Internal macro used by \ref X_PP_JOIN.
/// \sa X_PP_JOIN


/// \def X_PP_JOIN_IMPL
/// \brief Internal macro used by \ref X_PP_JOIN.
/// \sa X_PP_JOIN


















/// \def X_PP_JOIN
/// \ingroup Preprocessor
/// \brief Concatenates tokens, even when the tokens are macros themselves.
/// \details Because of the way the preprocessor works, macros can only be concatenated when using extra indirections.
/// This is being taken care of by the \ref X_PP_JOIN macro, and is mostly needed when dealing with macros where
/// one does not know whether it will be fed just simple tokens, or names of other macros as well.
/// Furthermore, because the preprocessor module has support for counting the number of arguments passed to a variadic
/// macro, the user does not need to call different macros based on the number of arguments given.
/// \code
///   #define MY_JOIN(a, b)				a##b
///   #define WORLD_NAME				world
///
///   // the token-pasting operator ## only works with simple tokens, not macros
///   MY_JOIN("hello", "world");			// outputs "hello""world"
///   MY_JOIN("hello", WORLD_NAME);			// outputs "hello"WORLD_NAME
///
///   X_PP_JOIN("hello", "world");			// outputs "hello""world"
///   X_PP_JOIN("hello", WORLD_NAME);		// outputs "hello""world"
///
///   // the macro is able to join any given number of arguments
///   X_PP_JOIN(a, b, c);					// outputs abc
///   X_PP_JOIN(a, b, c, d);				// outputs abcd
///   X_PP_JOIN(a, b, c, d, e);			// outputs abcde
/// \endcode
/// Note that no matter how many arguments we provide, the macro to use is always \ref X_PP_JOIN. This offers a powerful
/// facility for concatenating an unlimited amount of tokens.












#pragma once





























/// \def X_PP_LIST
/// \ingroup Preprocessor
/// \brief Expands the arguments so that a user-defined operation is called N number of times.
/// \details This macro is useful for e.g. generating a list of arguments based on a predefined count.
///
/// The operation to call must be a macro with one parameter, which denotes how many times the operation has been called.
///
/// Check the provided code example for example usage:
/// \code
///   // assume that we want to define a function Invoke with different overloads for 0, 1, 2, etc. arguments
///   // define our operation macro
///   #define ARGS(n)					ARG##n arg##n
///
///   // assume COUNT has been defined to 0, 1, 2, etc.
///   void Invoke(X_PP_LIST(COUNT, ARGS_IMPL))
///
///   // based on the definition of COUNT, the output will be one of the following
///   void Invoke()
///   void Invoke(ARG0 arg0)
///   void Invoke(ARG0 arg0, ARG1 arg1)
///   void Invoke(ARG0 arg0, ARG1 arg1, ARG2 arg2)
///   ...
/// \endcode






#pragma once
























/// \def X_PP_TO_BOOL
/// \ingroup Preprocessor
/// \brief Converts a condition into either 1 if the condition is true, or 0 otherwise.
/// \details The condition can either be a boolean value, or an integer.
/// \code
///   // boolean values
///   X_PP_TO_BOOL(false)			outputs 0
///   X_PP_TO_BOOL(true)			outputs 1
///
///   // integer values
///   X_PP_TO_BOOL(0)				outputs 0
///   X_PP_TO_BOOL(1)				outputs 1
///   X_PP_TO_BOOL(2)				outputs 1
///   X_PP_TO_BOOL(3)				outputs 1
/// \endcode






X_NAMESPACE_BEGIN(core)

template <typename T>
class Delegate {};















template<typename R  >
class Delegate<R()>
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
    typedef traits::Function<R(InstancePtr  )> InternalFunction;

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
    template<R (*Function)()>
    static X_INLINE R FunctionStub(InstancePtr  )
    {
        // we don't need the instance pointer because we're dealing with free functions
        return (Function)();
    }

    /// Internal function used when binding delegates to non-const member functions.
    template<class C, R (C::*Function)()>
    static X_INLINE R ClassMethodStub(InstancePtr instance  )
    {
        // cast the instance pointer back into the original class instance. this does not compromise type-safety
        // because we always know the concrete class type, given as a template argument.
        return (static_cast<C*>(instance.as_void)->*Function)();
    }

    /// Internal function used when binding delegates to const member functions.
    template<class C, R (C::*Function)() const>
    static X_INLINE R ConstClassMethodStub(InstancePtr instance  )
    {
        // cast the instance pointer back into the original class instance. this does not compromise type-safety
        // because we always know the concrete class type, given as a template argument.
        return (static_cast<const C*>(instance.as_const_void)->*Function)();
    }

public:
    /// Internal wrapper class used as a helper in Bind() overload resolution for non-const member functions.
    template<class C, R (C::*Function)()>
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
    template<class C, R (C::*Function)() const>
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
    template<R (*Function)()>
    void Bind(void)
    {
        stub_.instance.as_void = nullptr;
        stub_.function = &FunctionStub<Function>;
    }

    /// Binds a non-const class method to the delegate.
    template<class C, R (C::*Function)()>
    void Bind(NonConstWrapper<C, Function> wrapper)
    {
        stub_.instance.as_void = wrapper.instance_;
        stub_.function = &ClassMethodStub<C, Function>;
    }

    /// Binds a const class method to the delegate.
    template<class C, R (C::*Function)() const>
    void Bind(ConstWrapper<C, Function> wrapper)
    {
        stub_.instance.as_const_void = wrapper.instance_;
        stub_.function = &ConstClassMethodStub<C, Function>;
    }

    /// Invokes the delegate.
    R Invoke() const
    {
        X_ASSERT(stub_.function != nullptr, "Cannot invoke unbound delegate. Call Bind() first.")();
        return stub_.function(stub_.instance  );
    }

    X_INLINE operator bool() const
    {
        return stub_.function != nullptr;
    }

private:
    Stub stub_;
};


























template<typename R , typename ARG0>
class Delegate<R(ARG0)>
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
    typedef traits::Function<R(InstancePtr , ARG0)> InternalFunction;

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
    template<R (*Function)(ARG0)>
    static X_INLINE R FunctionStub(InstancePtr , ARG0 arg0)
    {
        // we don't need the instance pointer because we're dealing with free functions
        return (Function)(arg0);
    }

    /// Internal function used when binding delegates to non-const member functions.
    template<class C, R (C::*Function)(ARG0)>
    static X_INLINE R ClassMethodStub(InstancePtr instance , ARG0 arg0)
    {
        // cast the instance pointer back into the original class instance. this does not compromise type-safety
        // because we always know the concrete class type, given as a template argument.
        return (static_cast<C*>(instance.as_void)->*Function)(arg0);
    }

    /// Internal function used when binding delegates to const member functions.
    template<class C, R (C::*Function)(ARG0) const>
    static X_INLINE R ConstClassMethodStub(InstancePtr instance , ARG0 arg0)
    {
        // cast the instance pointer back into the original class instance. this does not compromise type-safety
        // because we always know the concrete class type, given as a template argument.
        return (static_cast<const C*>(instance.as_const_void)->*Function)(arg0);
    }

public:
    /// Internal wrapper class used as a helper in Bind() overload resolution for non-const member functions.
    template<class C, R (C::*Function)(ARG0)>
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
    template<class C, R (C::*Function)(ARG0) const>
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
    template<R (*Function)(ARG0)>
    void Bind(void)
    {
        stub_.instance.as_void = nullptr;
        stub_.function = &FunctionStub<Function>;
    }

    /// Binds a non-const class method to the delegate.
    template<class C, R (C::*Function)(ARG0)>
    void Bind(NonConstWrapper<C, Function> wrapper)
    {
        stub_.instance.as_void = wrapper.instance_;
        stub_.function = &ClassMethodStub<C, Function>;
    }

    /// Binds a const class method to the delegate.
    template<class C, R (C::*Function)(ARG0) const>
    void Bind(ConstWrapper<C, Function> wrapper)
    {
        stub_.instance.as_const_void = wrapper.instance_;
        stub_.function = &ConstClassMethodStub<C, Function>;
    }

    /// Invokes the delegate.
    R Invoke(ARG0 arg0) const
    {
        X_ASSERT(stub_.function != nullptr, "Cannot invoke unbound delegate. Call Bind() first.")();
        return stub_.function(stub_.instance , arg0);
    }

    X_INLINE operator bool() const
    {
        return stub_.function != nullptr;
    }

private:
    Stub stub_;
};


























template<typename R , typename ARG0, typename ARG1>
class Delegate<R(ARG0, ARG1)>
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
    typedef traits::Function<R(InstancePtr , ARG0, ARG1)> InternalFunction;

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
    template<R (*Function)(ARG0, ARG1)>
    static X_INLINE R FunctionStub(InstancePtr , ARG0 arg0, ARG1 arg1)
    {
        // we don't need the instance pointer because we're dealing with free functions
        return (Function)(arg0, arg1);
    }

    /// Internal function used when binding delegates to non-const member functions.
    template<class C, R (C::*Function)(ARG0, ARG1)>
    static X_INLINE R ClassMethodStub(InstancePtr instance , ARG0 arg0, ARG1 arg1)
    {
        // cast the instance pointer back into the original class instance. this does not compromise type-safety
        // because we always know the concrete class type, given as a template argument.
        return (static_cast<C*>(instance.as_void)->*Function)(arg0, arg1);
    }

    /// Internal function used when binding delegates to const member functions.
    template<class C, R (C::*Function)(ARG0, ARG1) const>
    static X_INLINE R ConstClassMethodStub(InstancePtr instance , ARG0 arg0, ARG1 arg1)
    {
        // cast the instance pointer back into the original class instance. this does not compromise type-safety
        // because we always know the concrete class type, given as a template argument.
        return (static_cast<const C*>(instance.as_const_void)->*Function)(arg0, arg1);
    }

public:
    /// Internal wrapper class used as a helper in Bind() overload resolution for non-const member functions.
    template<class C, R (C::*Function)(ARG0, ARG1)>
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
    template<class C, R (C::*Function)(ARG0, ARG1) const>
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
    template<R (*Function)(ARG0, ARG1)>
    void Bind(void)
    {
        stub_.instance.as_void = nullptr;
        stub_.function = &FunctionStub<Function>;
    }

    /// Binds a non-const class method to the delegate.
    template<class C, R (C::*Function)(ARG0, ARG1)>
    void Bind(NonConstWrapper<C, Function> wrapper)
    {
        stub_.instance.as_void = wrapper.instance_;
        stub_.function = &ClassMethodStub<C, Function>;
    }

    /// Binds a const class method to the delegate.
    template<class C, R (C::*Function)(ARG0, ARG1) const>
    void Bind(ConstWrapper<C, Function> wrapper)
    {
        stub_.instance.as_const_void = wrapper.instance_;
        stub_.function = &ConstClassMethodStub<C, Function>;
    }

    /// Invokes the delegate.
    R Invoke(ARG0 arg0, ARG1 arg1) const
    {
        X_ASSERT(stub_.function != nullptr, "Cannot invoke unbound delegate. Call Bind() first.")();
        return stub_.function(stub_.instance , arg0, arg1);
    }

    X_INLINE operator bool() const
    {
        return stub_.function != nullptr;
    }

private:
    Stub stub_;
};


























template<typename R , typename ARG0, typename ARG1, typename ARG2>
class Delegate<R(ARG0, ARG1, ARG2)>
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
    typedef traits::Function<R(InstancePtr , ARG0, ARG1, ARG2)> InternalFunction;

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
    template<R (*Function)(ARG0, ARG1, ARG2)>
    static X_INLINE R FunctionStub(InstancePtr , ARG0 arg0, ARG1 arg1, ARG2 arg2)
    {
        // we don't need the instance pointer because we're dealing with free functions
        return (Function)(arg0, arg1, arg2);
    }

    /// Internal function used when binding delegates to non-const member functions.
    template<class C, R (C::*Function)(ARG0, ARG1, ARG2)>
    static X_INLINE R ClassMethodStub(InstancePtr instance , ARG0 arg0, ARG1 arg1, ARG2 arg2)
    {
        // cast the instance pointer back into the original class instance. this does not compromise type-safety
        // because we always know the concrete class type, given as a template argument.
        return (static_cast<C*>(instance.as_void)->*Function)(arg0, arg1, arg2);
    }

    /// Internal function used when binding delegates to const member functions.
    template<class C, R (C::*Function)(ARG0, ARG1, ARG2) const>
    static X_INLINE R ConstClassMethodStub(InstancePtr instance , ARG0 arg0, ARG1 arg1, ARG2 arg2)
    {
        // cast the instance pointer back into the original class instance. this does not compromise type-safety
        // because we always know the concrete class type, given as a template argument.
        return (static_cast<const C*>(instance.as_const_void)->*Function)(arg0, arg1, arg2);
    }

public:
    /// Internal wrapper class used as a helper in Bind() overload resolution for non-const member functions.
    template<class C, R (C::*Function)(ARG0, ARG1, ARG2)>
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
    template<class C, R (C::*Function)(ARG0, ARG1, ARG2) const>
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
    template<R (*Function)(ARG0, ARG1, ARG2)>
    void Bind(void)
    {
        stub_.instance.as_void = nullptr;
        stub_.function = &FunctionStub<Function>;
    }

    /// Binds a non-const class method to the delegate.
    template<class C, R (C::*Function)(ARG0, ARG1, ARG2)>
    void Bind(NonConstWrapper<C, Function> wrapper)
    {
        stub_.instance.as_void = wrapper.instance_;
        stub_.function = &ClassMethodStub<C, Function>;
    }

    /// Binds a const class method to the delegate.
    template<class C, R (C::*Function)(ARG0, ARG1, ARG2) const>
    void Bind(ConstWrapper<C, Function> wrapper)
    {
        stub_.instance.as_const_void = wrapper.instance_;
        stub_.function = &ConstClassMethodStub<C, Function>;
    }

    /// Invokes the delegate.
    R Invoke(ARG0 arg0, ARG1 arg1, ARG2 arg2) const
    {
        X_ASSERT(stub_.function != nullptr, "Cannot invoke unbound delegate. Call Bind() first.")();
        return stub_.function(stub_.instance , arg0, arg1, arg2);
    }

    X_INLINE operator bool() const
    {
        return stub_.function != nullptr;
    }

private:
    Stub stub_;
};


























template<typename R , typename ARG0, typename ARG1, typename ARG2, typename ARG3>
class Delegate<R(ARG0, ARG1, ARG2, ARG3)>
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
    typedef traits::Function<R(InstancePtr , ARG0, ARG1, ARG2, ARG3)> InternalFunction;

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
    template<R (*Function)(ARG0, ARG1, ARG2, ARG3)>
    static X_INLINE R FunctionStub(InstancePtr , ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3)
    {
        // we don't need the instance pointer because we're dealing with free functions
        return (Function)(arg0, arg1, arg2, arg3);
    }

    /// Internal function used when binding delegates to non-const member functions.
    template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3)>
    static X_INLINE R ClassMethodStub(InstancePtr instance , ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3)
    {
        // cast the instance pointer back into the original class instance. this does not compromise type-safety
        // because we always know the concrete class type, given as a template argument.
        return (static_cast<C*>(instance.as_void)->*Function)(arg0, arg1, arg2, arg3);
    }

    /// Internal function used when binding delegates to const member functions.
    template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3) const>
    static X_INLINE R ConstClassMethodStub(InstancePtr instance , ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3)
    {
        // cast the instance pointer back into the original class instance. this does not compromise type-safety
        // because we always know the concrete class type, given as a template argument.
        return (static_cast<const C*>(instance.as_const_void)->*Function)(arg0, arg1, arg2, arg3);
    }

public:
    /// Internal wrapper class used as a helper in Bind() overload resolution for non-const member functions.
    template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3)>
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
    template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3) const>
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
    template<R (*Function)(ARG0, ARG1, ARG2, ARG3)>
    void Bind(void)
    {
        stub_.instance.as_void = nullptr;
        stub_.function = &FunctionStub<Function>;
    }

    /// Binds a non-const class method to the delegate.
    template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3)>
    void Bind(NonConstWrapper<C, Function> wrapper)
    {
        stub_.instance.as_void = wrapper.instance_;
        stub_.function = &ClassMethodStub<C, Function>;
    }

    /// Binds a const class method to the delegate.
    template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3) const>
    void Bind(ConstWrapper<C, Function> wrapper)
    {
        stub_.instance.as_const_void = wrapper.instance_;
        stub_.function = &ConstClassMethodStub<C, Function>;
    }

    /// Invokes the delegate.
    R Invoke(ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3) const
    {
        X_ASSERT(stub_.function != nullptr, "Cannot invoke unbound delegate. Call Bind() first.")();
        return stub_.function(stub_.instance , arg0, arg1, arg2, arg3);
    }

    X_INLINE operator bool() const
    {
        return stub_.function != nullptr;
    }

private:
    Stub stub_;
};


























template<typename R , typename ARG0, typename ARG1, typename ARG2, typename ARG3, typename ARG4>
class Delegate<R(ARG0, ARG1, ARG2, ARG3, ARG4)>
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
    typedef traits::Function<R(InstancePtr , ARG0, ARG1, ARG2, ARG3, ARG4)> InternalFunction;

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
    template<R (*Function)(ARG0, ARG1, ARG2, ARG3, ARG4)>
    static X_INLINE R FunctionStub(InstancePtr , ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4)
    {
        // we don't need the instance pointer because we're dealing with free functions
        return (Function)(arg0, arg1, arg2, arg3, arg4);
    }

    /// Internal function used when binding delegates to non-const member functions.
    template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4)>
    static X_INLINE R ClassMethodStub(InstancePtr instance , ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4)
    {
        // cast the instance pointer back into the original class instance. this does not compromise type-safety
        // because we always know the concrete class type, given as a template argument.
        return (static_cast<C*>(instance.as_void)->*Function)(arg0, arg1, arg2, arg3, arg4);
    }

    /// Internal function used when binding delegates to const member functions.
    template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4) const>
    static X_INLINE R ConstClassMethodStub(InstancePtr instance , ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4)
    {
        // cast the instance pointer back into the original class instance. this does not compromise type-safety
        // because we always know the concrete class type, given as a template argument.
        return (static_cast<const C*>(instance.as_const_void)->*Function)(arg0, arg1, arg2, arg3, arg4);
    }

public:
    /// Internal wrapper class used as a helper in Bind() overload resolution for non-const member functions.
    template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4)>
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
    template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4) const>
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
    template<R (*Function)(ARG0, ARG1, ARG2, ARG3, ARG4)>
    void Bind(void)
    {
        stub_.instance.as_void = nullptr;
        stub_.function = &FunctionStub<Function>;
    }

    /// Binds a non-const class method to the delegate.
    template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4)>
    void Bind(NonConstWrapper<C, Function> wrapper)
    {
        stub_.instance.as_void = wrapper.instance_;
        stub_.function = &ClassMethodStub<C, Function>;
    }

    /// Binds a const class method to the delegate.
    template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4) const>
    void Bind(ConstWrapper<C, Function> wrapper)
    {
        stub_.instance.as_const_void = wrapper.instance_;
        stub_.function = &ConstClassMethodStub<C, Function>;
    }

    /// Invokes the delegate.
    R Invoke(ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4) const
    {
        X_ASSERT(stub_.function != nullptr, "Cannot invoke unbound delegate. Call Bind() first.")();
        return stub_.function(stub_.instance , arg0, arg1, arg2, arg3, arg4);
    }

    X_INLINE operator bool() const
    {
        return stub_.function != nullptr;
    }

private:
    Stub stub_;
};


























template<typename R , typename ARG0, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5>
class Delegate<R(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5)>
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
    typedef traits::Function<R(InstancePtr , ARG0, ARG1, ARG2, ARG3, ARG4, ARG5)> InternalFunction;

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
    template<R (*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5)>
    static X_INLINE R FunctionStub(InstancePtr , ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5)
    {
        // we don't need the instance pointer because we're dealing with free functions
        return (Function)(arg0, arg1, arg2, arg3, arg4, arg5);
    }

    /// Internal function used when binding delegates to non-const member functions.
    template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5)>
    static X_INLINE R ClassMethodStub(InstancePtr instance , ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5)
    {
        // cast the instance pointer back into the original class instance. this does not compromise type-safety
        // because we always know the concrete class type, given as a template argument.
        return (static_cast<C*>(instance.as_void)->*Function)(arg0, arg1, arg2, arg3, arg4, arg5);
    }

    /// Internal function used when binding delegates to const member functions.
    template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5) const>
    static X_INLINE R ConstClassMethodStub(InstancePtr instance , ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5)
    {
        // cast the instance pointer back into the original class instance. this does not compromise type-safety
        // because we always know the concrete class type, given as a template argument.
        return (static_cast<const C*>(instance.as_const_void)->*Function)(arg0, arg1, arg2, arg3, arg4, arg5);
    }

public:
    /// Internal wrapper class used as a helper in Bind() overload resolution for non-const member functions.
    template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5)>
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
    template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5) const>
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
    template<R (*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5)>
    void Bind(void)
    {
        stub_.instance.as_void = nullptr;
        stub_.function = &FunctionStub<Function>;
    }

    /// Binds a non-const class method to the delegate.
    template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5)>
    void Bind(NonConstWrapper<C, Function> wrapper)
    {
        stub_.instance.as_void = wrapper.instance_;
        stub_.function = &ClassMethodStub<C, Function>;
    }

    /// Binds a const class method to the delegate.
    template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5) const>
    void Bind(ConstWrapper<C, Function> wrapper)
    {
        stub_.instance.as_const_void = wrapper.instance_;
        stub_.function = &ConstClassMethodStub<C, Function>;
    }

    /// Invokes the delegate.
    R Invoke(ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5) const
    {
        X_ASSERT(stub_.function != nullptr, "Cannot invoke unbound delegate. Call Bind() first.")();
        return stub_.function(stub_.instance , arg0, arg1, arg2, arg3, arg4, arg5);
    }

    X_INLINE operator bool() const
    {
        return stub_.function != nullptr;
    }

private:
    Stub stub_;
};


























template<typename R , typename ARG0, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6>
class Delegate<R(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6)>
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
    typedef traits::Function<R(InstancePtr , ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6)> InternalFunction;

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
    template<R (*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6)>
    static X_INLINE R FunctionStub(InstancePtr , ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6)
    {
        // we don't need the instance pointer because we're dealing with free functions
        return (Function)(arg0, arg1, arg2, arg3, arg4, arg5, arg6);
    }

    /// Internal function used when binding delegates to non-const member functions.
    template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6)>
    static X_INLINE R ClassMethodStub(InstancePtr instance , ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6)
    {
        // cast the instance pointer back into the original class instance. this does not compromise type-safety
        // because we always know the concrete class type, given as a template argument.
        return (static_cast<C*>(instance.as_void)->*Function)(arg0, arg1, arg2, arg3, arg4, arg5, arg6);
    }

    /// Internal function used when binding delegates to const member functions.
    template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6) const>
    static X_INLINE R ConstClassMethodStub(InstancePtr instance , ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6)
    {
        // cast the instance pointer back into the original class instance. this does not compromise type-safety
        // because we always know the concrete class type, given as a template argument.
        return (static_cast<const C*>(instance.as_const_void)->*Function)(arg0, arg1, arg2, arg3, arg4, arg5, arg6);
    }

public:
    /// Internal wrapper class used as a helper in Bind() overload resolution for non-const member functions.
    template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6)>
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
    template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6) const>
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
    template<R (*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6)>
    void Bind(void)
    {
        stub_.instance.as_void = nullptr;
        stub_.function = &FunctionStub<Function>;
    }

    /// Binds a non-const class method to the delegate.
    template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6)>
    void Bind(NonConstWrapper<C, Function> wrapper)
    {
        stub_.instance.as_void = wrapper.instance_;
        stub_.function = &ClassMethodStub<C, Function>;
    }

    /// Binds a const class method to the delegate.
    template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6) const>
    void Bind(ConstWrapper<C, Function> wrapper)
    {
        stub_.instance.as_const_void = wrapper.instance_;
        stub_.function = &ConstClassMethodStub<C, Function>;
    }

    /// Invokes the delegate.
    R Invoke(ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6) const
    {
        X_ASSERT(stub_.function != nullptr, "Cannot invoke unbound delegate. Call Bind() first.")();
        return stub_.function(stub_.instance , arg0, arg1, arg2, arg3, arg4, arg5, arg6);
    }

    X_INLINE operator bool() const
    {
        return stub_.function != nullptr;
    }

private:
    Stub stub_;
};


























template<typename R , typename ARG0, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6, typename ARG7>
class Delegate<R(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7)>
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
    typedef traits::Function<R(InstancePtr , ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7)> InternalFunction;

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
    template<R (*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7)>
    static X_INLINE R FunctionStub(InstancePtr , ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6, ARG7 arg7)
    {
        // we don't need the instance pointer because we're dealing with free functions
        return (Function)(arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7);
    }

    /// Internal function used when binding delegates to non-const member functions.
    template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7)>
    static X_INLINE R ClassMethodStub(InstancePtr instance , ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6, ARG7 arg7)
    {
        // cast the instance pointer back into the original class instance. this does not compromise type-safety
        // because we always know the concrete class type, given as a template argument.
        return (static_cast<C*>(instance.as_void)->*Function)(arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7);
    }

    /// Internal function used when binding delegates to const member functions.
    template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7) const>
    static X_INLINE R ConstClassMethodStub(InstancePtr instance , ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6, ARG7 arg7)
    {
        // cast the instance pointer back into the original class instance. this does not compromise type-safety
        // because we always know the concrete class type, given as a template argument.
        return (static_cast<const C*>(instance.as_const_void)->*Function)(arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7);
    }

public:
    /// Internal wrapper class used as a helper in Bind() overload resolution for non-const member functions.
    template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7)>
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
    template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7) const>
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
    template<R (*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7)>
    void Bind(void)
    {
        stub_.instance.as_void = nullptr;
        stub_.function = &FunctionStub<Function>;
    }

    /// Binds a non-const class method to the delegate.
    template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7)>
    void Bind(NonConstWrapper<C, Function> wrapper)
    {
        stub_.instance.as_void = wrapper.instance_;
        stub_.function = &ClassMethodStub<C, Function>;
    }

    /// Binds a const class method to the delegate.
    template<class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7) const>
    void Bind(ConstWrapper<C, Function> wrapper)
    {
        stub_.instance.as_const_void = wrapper.instance_;
        stub_.function = &ConstClassMethodStub<C, Function>;
    }

    /// Invokes the delegate.
    R Invoke(ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6, ARG7 arg7) const
    {
        X_ASSERT(stub_.function != nullptr, "Cannot invoke unbound delegate. Call Bind() first.")();
        return stub_.function(stub_.instance , arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7);
    }

    X_INLINE operator bool() const
    {
        return stub_.function != nullptr;
    }

private:
    Stub stub_;
};













X_NAMESPACE_END
