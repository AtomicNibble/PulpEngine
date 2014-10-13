#pragma once





X_NAMESPACE_BEGIN(core)

/// \ingroup Util
/// \brief A class that implements a delegate system.
/// \details The delegate system provided by this class is type-safe, very lightweight, and \b extremely efficient.
/// It does not use any virtual functions, and no dynamic memory allocations are made. The system supports free functions
/// as well as both const-qualified and non-const member functions.
///
/// Different template specializations allow delegates to be declared with comprehensible syntax, supporting completely
/// arbitrary function signatures. Delegate declaration is demonstrated in the following example:
/// \code
///   // declares a delegate taking no parameter, and returning nothing
///   typedef core::Delegate<void (void)> NoArgumentDelegate;
///
///   // declares a delegate taking a float and int parameter, returning an int
///   typedef core::Delegate<int (float, int)> ComplexDelegate;
/// \endcode
/// Before a delegate can be invoked, it needs to be bound to either a free function or member function of a class. The
/// following example shows how delegates are bound and invoked:
/// \code
///   // a free function that fits the delegate signature
///   int FreeFunction(float, int)
///   {
///     // do something
///     ...
///
///     return ...;
///   }
///
///   ComplexDelegate testDelegate;
///
///   // bind the delegate to the free function
///   testDelegate.Bind<&FreeFunction>();
///
///   // invoke the delegate
///   int value = testDelegate.Invoke(1.0f, 20);
/// \endcode
/// As can be seen, invoking a delegate looks and acts exactly like an ordinary function call, the only difference being
/// that any function can be bound to the delegate. The following example demonstrates a delegate that is bound to a member
/// function:
/// \code
///   struct TestClass
///   {
///     int Function(float, int)
///     {
///       // do something
///       ...
///
///       return ...;
///     }
///   };
///
///   TestClass instance;
///   ComplexDelegate testDelegate;
///
///   // delegates bound to member functions need a class instance upon which the call is made
///   testDelegate.Bind<TestClass, &TestClass::Function>(&instance);
///   int value = testDelegate.Invoke(1.0f, 20);
/// \endcode
/// Even though pointers-to-member-functions are more complex than ordinary pointers-to-functions in C++, the delegate
/// system implementation manages to achieve the exact same speed for member functions as for free functions.
///
/// For more details and an explanation of how the delegate system works internally, see http://molecularmusings.wordpress.com/2011/09/19/generic-type-safe-delegates-and-events-in-c/.
/// \remark The header file Delegate.h is built from Delegate.gen.h and Delegate.gen using an automatic pre-build step.
/// Do not attempt to change something in the Delegate.h header file directly - all changes will be lost.
/// \sa Event
template <typename T>
class Delegate {};



/// \brief Partial template specialization of the Delegate base template.
/// \details The template specialization is implemented using the preprocessor library, and built using
/// a custom pre-build step.
/// \sa Delegate
template <typename R  >
class Delegate<R()>
{
	/// Internal union that can hold a pointer to both non-const and const class instances.
	union InstancePtr
	{
		/// Default constructor, initializing to \c nullptr.
		inline InstancePtr(void)
		: as_void(nullptr)
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
	typedef traits::Function<R(InstancePtr)> InternalFunction;

	/// \brief Internal stub that holds both a class instance (if any) and a pointer to the function which is called
	/// when the delegate is invoked.
	struct Stub
	{
		/// Default constructor, initializing to \c nullptr.
		inline Stub(void)
		: instance()
		, function(nullptr)
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
	template <R(*Function)()>
	static X_INLINE R FunctionStub(InstancePtr)
	{
		// we don't need the instance pointer because we're dealing with free functions
		return (Function)();
	}

	/// Internal function used when binding delegates to non-const member functions.
	template <class C, R(C::*Function)()>
	static X_INLINE R ClassMethodStub(InstancePtr instance)
	{
		// cast the instance pointer back into the original class instance. this does not compromise type-safety
		// because we always know the concrete class type, given as a template argument.
		return (static_cast<C*>(instance.as_void)->*Function)();
	}

	/// Internal function used when binding delegates to const member functions.
	template <class C, R(C::*Function)() const>
	static X_INLINE R ConstClassMethodStub(InstancePtr instance)
	{
		// cast the instance pointer back into the original class instance. this does not compromise type-safety
		// because we always know the concrete class type, given as a template argument.
		return (static_cast<const C*>(instance.as_const_void)->*Function)();
	}

public:
	/// Internal wrapper class used as a helper in Bind() overload resolution for non-const member functions.
	template <class C, R(C::*Function)()>
	struct NonConstWrapper
	{
		/// Default constructor.
		NonConstWrapper(C* instance)
		: m_instance(instance)
		{
		}

		C* m_instance;
	};

	/// Internal wrapper class used as a helper in Bind() overload resolution for const member functions.
	template <class C, R(C::*Function)() const>
	struct ConstWrapper
	{
		/// Default constructor.
		ConstWrapper(const C* instance)
		: m_instance(instance)
		{
		}

		const C* m_instance;
	};


	/// Default constructor.
	Delegate(void)
		: m_stub()
	{
	}

	/// Binds a free function to the delegate.
	template <R(*Function)()>
	void Bind(void)
	{
		m_stub.instance.as_void = nullptr;
		m_stub.function = &FunctionStub<Function>;
	}

	/// Binds a non-const class method to the delegate.
	template <class C, R(C::*Function)()>
	void Bind(NonConstWrapper<C, Function> wrapper)
	{
		m_stub.instance.as_void = wrapper.m_instance;
		m_stub.function = &ClassMethodStub<C, Function>;
	}

	/// Binds a const class method to the delegate.
	template <class C, R(C::*Function)() const>
	void Bind(ConstWrapper<C, Function> wrapper)
	{
		m_stub.instance.as_const_void = wrapper.m_instance;
		m_stub.function = &ConstClassMethodStub<C, Function>;
	}

	/// Invokes the delegate.
	R Invoke() const
	{
		X_ASSERT(m_stub.function != nullptr, "Cannot invoke unbound delegate. Call Bind() first.")();
		return m_stub.function(m_stub.instance);
	}

private:
	Stub m_stub;
};


/// \brief Partial template specialization of the Delegate base template.
/// \details The template specialization is implemented using the preprocessor library, and built using
/// a custom pre-build step.
/// \sa Delegate
template <typename R, typename ARG0>
class Delegate<R(ARG0)>
{
	/// Internal union that can hold a pointer to both non-const and const class instances.
	union InstancePtr
	{
		/// Default constructor, initializing to \c nullptr.
		inline InstancePtr(void)
		: as_void(nullptr)
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
	typedef traits::Function<R(InstancePtr, ARG0)> InternalFunction;

	/// \brief Internal stub that holds both a class instance (if any) and a pointer to the function which is called
	/// when the delegate is invoked.
	struct Stub
	{
		/// Default constructor, initializing to \c nullptr.
		inline Stub(void)
		: instance()
		, function(nullptr)
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
	template <R(*Function)(ARG0)>
	static X_INLINE R FunctionStub(InstancePtr, ARG0 arg0)
	{
		// we don't need the instance pointer because we're dealing with free functions
		return (Function)(arg0);
	}

	/// Internal function used when binding delegates to non-const member functions.
	template <class C, R(C::*Function)(ARG0)>
	static X_INLINE R ClassMethodStub(InstancePtr instance, ARG0 arg0)
	{
		// cast the instance pointer back into the original class instance. this does not compromise type-safety
		// because we always know the concrete class type, given as a template argument.
		return (static_cast<C*>(instance.as_void)->*Function)(arg0);
	}

	/// Internal function used when binding delegates to const member functions.
	template <class C, R(C::*Function)(ARG0) const>
	static X_INLINE R ConstClassMethodStub(InstancePtr instance, ARG0 arg0)
	{
		// cast the instance pointer back into the original class instance. this does not compromise type-safety
		// because we always know the concrete class type, given as a template argument.
		return (static_cast<const C*>(instance.as_const_void)->*Function)(arg0);
	}

public:
	/// Internal wrapper class used as a helper in Bind() overload resolution for non-const member functions.
	template <class C, R(C::*Function)(ARG0)>
	struct NonConstWrapper
	{
		/// Default constructor.
		NonConstWrapper(C* instance)
		: m_instance(instance)
		{
		}

		C* m_instance;
	};

	/// Internal wrapper class used as a helper in Bind() overload resolution for const member functions.
	template <class C, R(C::*Function)(ARG0) const>
	struct ConstWrapper
	{
		/// Default constructor.
		ConstWrapper(const C* instance)
		: m_instance(instance)
		{
		}

		const C* m_instance;
	};


	/// Default constructor.
	Delegate(void)
		: m_stub()
	{
	}

	/// Binds a free function to the delegate.
	template <R(*Function)(ARG0)>
	void Bind(void)
	{
		m_stub.instance.as_void = nullptr;
		m_stub.function = &FunctionStub<Function>;
	}

	/// Binds a non-const class method to the delegate.
	template <class C, R(C::*Function)(ARG0)>
	void Bind(NonConstWrapper<C, Function> wrapper)
	{
		m_stub.instance.as_void = wrapper.m_instance;
		m_stub.function = &ClassMethodStub<C, Function>;
	}

	/// Binds a const class method to the delegate.
	template <class C, R(C::*Function)(ARG0) const>
	void Bind(ConstWrapper<C, Function> wrapper)
	{
		m_stub.instance.as_const_void = wrapper.m_instance;
		m_stub.function = &ConstClassMethodStub<C, Function>;
	}

	/// Invokes the delegate.
	R Invoke(ARG0 arg0) const
	{
		X_ASSERT(m_stub.function != nullptr, "Cannot invoke unbound delegate. Call Bind() first.")();
		return m_stub.function(m_stub.instance, arg0);
	}

	operator bool() const { return m_stub.function != nullptr; }

private:
	Stub m_stub;
};



/// \brief Partial template specialization of the Delegate base template.
/// \details The template specialization is implemented using the preprocessor library, and built using
/// a custom pre-build step.
/// \sa Delegate
template <typename R, typename ARG0, typename ARG1>
class Delegate<R(ARG0, ARG1)>
{
	/// Internal union that can hold a pointer to both non-const and const class instances.
	union InstancePtr
	{
		/// Default constructor, initializing to \c nullptr.
		inline InstancePtr(void)
		: as_void(nullptr)
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
	typedef traits::Function<R(InstancePtr, ARG0, ARG1)> InternalFunction;

	/// \brief Internal stub that holds both a class instance (if any) and a pointer to the function which is called
	/// when the delegate is invoked.
	struct Stub
	{
		/// Default constructor, initializing to \c nullptr.
		inline Stub(void)
		: instance()
		, function(nullptr)
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
	template <R(*Function)(ARG0, ARG1)>
	static X_INLINE R FunctionStub(InstancePtr, ARG0 arg0, ARG1 arg1)
	{
		// we don't need the instance pointer because we're dealing with free functions
		return (Function)(arg0, arg1);
	}

	/// Internal function used when binding delegates to non-const member functions.
	template <class C, R(C::*Function)(ARG0, ARG1)>
	static X_INLINE R ClassMethodStub(InstancePtr instance, ARG0 arg0, ARG1 arg1)
	{
		// cast the instance pointer back into the original class instance. this does not compromise type-safety
		// because we always know the concrete class type, given as a template argument.
		return (static_cast<C*>(instance.as_void)->*Function)(arg0, arg1);
	}

	/// Internal function used when binding delegates to const member functions.
	template <class C, R(C::*Function)(ARG0, ARG1) const>
	static X_INLINE R ConstClassMethodStub(InstancePtr instance, ARG0 arg0, ARG1 arg1)
	{
		// cast the instance pointer back into the original class instance. this does not compromise type-safety
		// because we always know the concrete class type, given as a template argument.
		return (static_cast<const C*>(instance.as_const_void)->*Function)(arg0, arg1);
	}

public:
	/// Internal wrapper class used as a helper in Bind() overload resolution for non-const member functions.
	template <class C, R(C::*Function)(ARG0, ARG1)>
	struct NonConstWrapper
	{
		/// Default constructor.
		NonConstWrapper(C* instance)
		: m_instance(instance)
		{
		}

		C* m_instance;
	};

	/// Internal wrapper class used as a helper in Bind() overload resolution for const member functions.
	template <class C, R(C::*Function)(ARG0, ARG1) const>
	struct ConstWrapper
	{
		/// Default constructor.
		ConstWrapper(const C* instance)
		: m_instance(instance)
		{
		}

		const C* m_instance;
	};


	/// Default constructor.
	Delegate(void)
		: m_stub()
	{
	}

	/// Binds a free function to the delegate.
	template <R(*Function)(ARG0, ARG1)>
	void Bind(void)
	{
		m_stub.instance.as_void = nullptr;
		m_stub.function = &FunctionStub<Function>;
	}

	/// Binds a non-const class method to the delegate.
	template <class C, R(C::*Function)(ARG0, ARG1)>
	void Bind(NonConstWrapper<C, Function> wrapper)
	{
		m_stub.instance.as_void = wrapper.m_instance;
		m_stub.function = &ClassMethodStub<C, Function>;
	}

	/// Binds a const class method to the delegate.
	template <class C, R(C::*Function)(ARG0, ARG1) const>
	void Bind(ConstWrapper<C, Function> wrapper)
	{
		m_stub.instance.as_const_void = wrapper.m_instance;
		m_stub.function = &ConstClassMethodStub<C, Function>;
	}

	/// Invokes the delegate.
	R Invoke(ARG0 arg0, ARG1 arg1) const
	{
		X_ASSERT(m_stub.function != nullptr, "Cannot invoke unbound delegate. Call Bind() first.")();
		return m_stub.function(m_stub.instance, arg0, arg1);
	}

private:
	Stub m_stub;
};


/// \brief Partial template specialization of the Delegate base template.
/// \details The template specialization is implemented using the preprocessor library, and built using
/// a custom pre-build step.
/// \sa Delegate
template <typename R, typename ARG0, typename ARG1, typename ARG2>
class Delegate<R(ARG0, ARG1, ARG2)>
{
	/// Internal union that can hold a pointer to both non-const and const class instances.
	union InstancePtr
	{
		/// Default constructor, initializing to \c nullptr.
		inline InstancePtr(void)
		: as_void(nullptr)
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
	typedef traits::Function<R(InstancePtr, ARG0, ARG1, ARG2)> InternalFunction;

	/// \brief Internal stub that holds both a class instance (if any) and a pointer to the function which is called
	/// when the delegate is invoked.
	struct Stub
	{
		/// Default constructor, initializing to \c nullptr.
		inline Stub(void)
		: instance()
		, function(nullptr)
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
	template <R(*Function)(ARG0, ARG1, ARG2)>
	static X_INLINE R FunctionStub(InstancePtr, ARG0 arg0, ARG1 arg1, ARG2 arg2)
	{
		// we don't need the instance pointer because we're dealing with free functions
		return (Function)(arg0, arg1, arg2);
	}

	/// Internal function used when binding delegates to non-const member functions.
	template <class C, R(C::*Function)(ARG0, ARG1, ARG2)>
	static X_INLINE R ClassMethodStub(InstancePtr instance, ARG0 arg0, ARG1 arg1, ARG2 arg2)
	{
		// cast the instance pointer back into the original class instance. this does not compromise type-safety
		// because we always know the concrete class type, given as a template argument.
		return (static_cast<C*>(instance.as_void)->*Function)(arg0, arg1, arg2);
	}

	/// Internal function used when binding delegates to const member functions.
	template <class C, R(C::*Function)(ARG0, ARG1, ARG2) const>
	static X_INLINE R ConstClassMethodStub(InstancePtr instance, ARG0 arg0, ARG1 arg1, ARG2 arg2)
	{
		// cast the instance pointer back into the original class instance. this does not compromise type-safety
		// because we always know the concrete class type, given as a template argument.
		return (static_cast<const C*>(instance.as_const_void)->*Function)(arg0, arg1, arg2);
	}

public:
	/// Internal wrapper class used as a helper in Bind() overload resolution for non-const member functions.
	template <class C, R(C::*Function)(ARG0, ARG1, ARG2)>
	struct NonConstWrapper
	{
		/// Default constructor.
		NonConstWrapper(C* instance)
		: m_instance(instance)
		{
		}

		C* m_instance;
	};

	/// Internal wrapper class used as a helper in Bind() overload resolution for const member functions.
	template <class C, R(C::*Function)(ARG0, ARG1, ARG2) const>
	struct ConstWrapper
	{
		/// Default constructor.
		ConstWrapper(const C* instance)
		: m_instance(instance)
		{
		}

		const C* m_instance;
	};


	/// Default constructor.
	Delegate(void)
		: m_stub()
	{
	}

	/// Binds a free function to the delegate.
	template <R(*Function)(ARG0, ARG1, ARG2)>
	void Bind(void)
	{
		m_stub.instance.as_void = nullptr;
		m_stub.function = &FunctionStub<Function>;
	}

	/// Binds a non-const class method to the delegate.
	template <class C, R(C::*Function)(ARG0, ARG1, ARG2)>
	void Bind(NonConstWrapper<C, Function> wrapper)
	{
		m_stub.instance.as_void = wrapper.m_instance;
		m_stub.function = &ClassMethodStub<C, Function>;
	}

	/// Binds a const class method to the delegate.
	template <class C, R(C::*Function)(ARG0, ARG1, ARG2) const>
	void Bind(ConstWrapper<C, Function> wrapper)
	{
		m_stub.instance.as_const_void = wrapper.m_instance;
		m_stub.function = &ConstClassMethodStub<C, Function>;
	}

	/// Invokes the delegate.
	R Invoke(ARG0 arg0, ARG1 arg1, ARG2 arg2) const
	{
		X_ASSERT(m_stub.function != nullptr, "Cannot invoke unbound delegate. Call Bind() first.")();
		return m_stub.function(m_stub.instance, arg0, arg1, arg2);
	}

private:
	Stub m_stub;
};


/// \brief Partial template specialization of the Delegate base template.
/// \details The template specialization is implemented using the preprocessor library, and built using
/// a custom pre-build step.
/// \sa Delegate
template <typename R, typename ARG0, typename ARG1, typename ARG2, typename ARG3>
class Delegate<R(ARG0, ARG1, ARG2, ARG3)>
{
	/// Internal union that can hold a pointer to both non-const and const class instances.
	union InstancePtr
	{
		/// Default constructor, initializing to \c nullptr.
		inline InstancePtr(void)
		: as_void(nullptr)
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
	typedef traits::Function<R(InstancePtr, ARG0, ARG1, ARG2, ARG3)> InternalFunction;

	/// \brief Internal stub that holds both a class instance (if any) and a pointer to the function which is called
	/// when the delegate is invoked.
	struct Stub
	{
		/// Default constructor, initializing to \c nullptr.
		inline Stub(void)
		: instance()
		, function(nullptr)
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
	template <R(*Function)(ARG0, ARG1, ARG2, ARG3)>
	static X_INLINE R FunctionStub(InstancePtr, ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3)
	{
		// we don't need the instance pointer because we're dealing with free functions
		return (Function)(arg0, arg1, arg2, arg3);
	}

	/// Internal function used when binding delegates to non-const member functions.
	template <class C, R(C::*Function)(ARG0, ARG1, ARG2, ARG3)>
	static X_INLINE R ClassMethodStub(InstancePtr instance, ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3)
	{
		// cast the instance pointer back into the original class instance. this does not compromise type-safety
		// because we always know the concrete class type, given as a template argument.
		return (static_cast<C*>(instance.as_void)->*Function)(arg0, arg1, arg2, arg3);
	}

	/// Internal function used when binding delegates to const member functions.
	template <class C, R(C::*Function)(ARG0, ARG1, ARG2, ARG3) const>
	static X_INLINE R ConstClassMethodStub(InstancePtr instance, ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3)
	{
		// cast the instance pointer back into the original class instance. this does not compromise type-safety
		// because we always know the concrete class type, given as a template argument.
		return (static_cast<const C*>(instance.as_const_void)->*Function)(arg0, arg1, arg2, arg3);
	}

public:
	/// Internal wrapper class used as a helper in Bind() overload resolution for non-const member functions.
	template <class C, R(C::*Function)(ARG0, ARG1, ARG2, ARG3)>
	struct NonConstWrapper
	{
		/// Default constructor.
		NonConstWrapper(C* instance)
		: m_instance(instance)
		{
		}

		C* m_instance;
	};

	/// Internal wrapper class used as a helper in Bind() overload resolution for const member functions.
	template <class C, R(C::*Function)(ARG0, ARG1, ARG2, ARG3) const>
	struct ConstWrapper
	{
		/// Default constructor.
		ConstWrapper(const C* instance)
		: m_instance(instance)
		{
		}

		const C* m_instance;
	};


	/// Default constructor.
	Delegate(void)
		: m_stub()
	{
	}

	/// Binds a free function to the delegate.
	template <R(*Function)(ARG0, ARG1, ARG2, ARG3)>
	void Bind(void)
	{
		m_stub.instance.as_void = nullptr;
		m_stub.function = &FunctionStub<Function>;
	}

	/// Binds a non-const class method to the delegate.
	template <class C, R(C::*Function)(ARG0, ARG1, ARG2, ARG3)>
	void Bind(NonConstWrapper<C, Function> wrapper)
	{
		m_stub.instance.as_void = wrapper.m_instance;
		m_stub.function = &ClassMethodStub<C, Function>;
	}

	/// Binds a const class method to the delegate.
	template <class C, R(C::*Function)(ARG0, ARG1, ARG2, ARG3) const>
	void Bind(ConstWrapper<C, Function> wrapper)
	{
		m_stub.instance.as_const_void = wrapper.m_instance;
		m_stub.function = &ConstClassMethodStub<C, Function>;
	}

	/// Invokes the delegate.
	R Invoke(ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3) const
	{
		X_ASSERT(m_stub.function != nullptr, "Cannot invoke unbound delegate. Call Bind() first.")();
		return m_stub.function(m_stub.instance, arg0, arg1, arg2, arg3);
	}

private:
	Stub m_stub;
};



/// \brief Partial template specialization of the Delegate base template.
/// \details The template specialization is implemented using the preprocessor library, and built using
/// a custom pre-build step.
/// \sa Delegate
template <typename R, typename ARG0, typename ARG1, typename ARG2, typename ARG3, typename ARG4>
class Delegate<R(ARG0, ARG1, ARG2, ARG3, ARG4)>
{
	/// Internal union that can hold a pointer to both non-const and const class instances.
	union InstancePtr
	{
		/// Default constructor, initializing to \c nullptr.
		inline InstancePtr(void)
		: as_void(nullptr)
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
	typedef traits::Function<R(InstancePtr, ARG0, ARG1, ARG2, ARG3, ARG4)> InternalFunction;

	/// \brief Internal stub that holds both a class instance (if any) and a pointer to the function which is called
	/// when the delegate is invoked.
	struct Stub
	{
		/// Default constructor, initializing to \c nullptr.
		inline Stub(void)
		: instance()
		, function(nullptr)
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
	template <R(*Function)(ARG0, ARG1, ARG2, ARG3, ARG4)>
	static X_INLINE R FunctionStub(InstancePtr, ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4)
	{
		// we don't need the instance pointer because we're dealing with free functions
		return (Function)(arg0, arg1, arg2, arg3, arg4);
	}

	/// Internal function used when binding delegates to non-const member functions.
	template <class C, R(C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4)>
	static X_INLINE R ClassMethodStub(InstancePtr instance, ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4)
	{
		// cast the instance pointer back into the original class instance. this does not compromise type-safety
		// because we always know the concrete class type, given as a template argument.
		return (static_cast<C*>(instance.as_void)->*Function)(arg0, arg1, arg2, arg3, arg4);
	}

	/// Internal function used when binding delegates to const member functions.
	template <class C, R(C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4) const>
	static X_INLINE R ConstClassMethodStub(InstancePtr instance, ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4)
	{
		// cast the instance pointer back into the original class instance. this does not compromise type-safety
		// because we always know the concrete class type, given as a template argument.
		return (static_cast<const C*>(instance.as_const_void)->*Function)(arg0, arg1, arg2, arg3, arg4);
	}

public:
	/// Internal wrapper class used as a helper in Bind() overload resolution for non-const member functions.
	template <class C, R(C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4)>
	struct NonConstWrapper
	{
		/// Default constructor.
		NonConstWrapper(C* instance)
		: m_instance(instance)
		{
		}

		C* m_instance;
	};

	/// Internal wrapper class used as a helper in Bind() overload resolution for const member functions.
	template <class C, R(C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4) const>
	struct ConstWrapper
	{
		/// Default constructor.
		ConstWrapper(const C* instance)
		: m_instance(instance)
		{
		}

		const C* m_instance;
	};


	/// Default constructor.
	Delegate(void)
		: m_stub()
	{
	}

	/// Binds a free function to the delegate.
	template <R(*Function)(ARG0, ARG1, ARG2, ARG3, ARG4)>
	void Bind(void)
	{
		m_stub.instance.as_void = nullptr;
		m_stub.function = &FunctionStub<Function>;
	}

	/// Binds a non-const class method to the delegate.
	template <class C, R(C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4)>
	void Bind(NonConstWrapper<C, Function> wrapper)
	{
		m_stub.instance.as_void = wrapper.m_instance;
		m_stub.function = &ClassMethodStub<C, Function>;
	}

	/// Binds a const class method to the delegate.
	template <class C, R(C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4) const>
	void Bind(ConstWrapper<C, Function> wrapper)
	{
		m_stub.instance.as_const_void = wrapper.m_instance;
		m_stub.function = &ConstClassMethodStub<C, Function>;
	}

	/// Invokes the delegate.
	R Invoke(ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4) const
	{
		X_ASSERT(m_stub.function != nullptr, "Cannot invoke unbound delegate. Call Bind() first.")();
		return m_stub.function(m_stub.instance, arg0, arg1, arg2, arg3, arg4);
	}

private:
	Stub m_stub;
};



/// \brief Partial template specialization of the Delegate base template.
/// \details The template specialization is implemented using the preprocessor library, and built using
/// a custom pre-build step.
/// \sa Delegate
template <typename R, typename ARG0, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5>
class Delegate<R(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5)>
{
	/// Internal union that can hold a pointer to both non-const and const class instances.
	union InstancePtr
	{
		/// Default constructor, initializing to \c nullptr.
		inline InstancePtr(void)
		: as_void(nullptr)
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
	typedef traits::Function<R(InstancePtr, ARG0, ARG1, ARG2, ARG3, ARG4, ARG5)> InternalFunction;

	/// \brief Internal stub that holds both a class instance (if any) and a pointer to the function which is called
	/// when the delegate is invoked.
	struct Stub
	{
		/// Default constructor, initializing to \c nullptr.
		inline Stub(void)
		: instance()
		, function(nullptr)
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
	template <R(*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5)>
	static X_INLINE R FunctionStub(InstancePtr, ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5)
	{
		// we don't need the instance pointer because we're dealing with free functions
		return (Function)(arg0, arg1, arg2, arg3, arg4, arg5);
	}

	/// Internal function used when binding delegates to non-const member functions.
	template <class C, R(C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5)>
	static X_INLINE R ClassMethodStub(InstancePtr instance, ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5)
	{
		// cast the instance pointer back into the original class instance. this does not compromise type-safety
		// because we always know the concrete class type, given as a template argument.
		return (static_cast<C*>(instance.as_void)->*Function)(arg0, arg1, arg2, arg3, arg4, arg5);
	}

	/// Internal function used when binding delegates to const member functions.
	template <class C, R(C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5) const>
	static X_INLINE R ConstClassMethodStub(InstancePtr instance, ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5)
	{
		// cast the instance pointer back into the original class instance. this does not compromise type-safety
		// because we always know the concrete class type, given as a template argument.
		return (static_cast<const C*>(instance.as_const_void)->*Function)(arg0, arg1, arg2, arg3, arg4, arg5);
	}

public:
	/// Internal wrapper class used as a helper in Bind() overload resolution for non-const member functions.
	template <class C, R(C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5)>
	struct NonConstWrapper
	{
		/// Default constructor.
		NonConstWrapper(C* instance)
		: m_instance(instance)
		{
		}

		C* m_instance;
	};

	/// Internal wrapper class used as a helper in Bind() overload resolution for const member functions.
	template <class C, R(C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5) const>
	struct ConstWrapper
	{
		/// Default constructor.
		ConstWrapper(const C* instance)
		: m_instance(instance)
		{
		}

		const C* m_instance;
	};


	/// Default constructor.
	Delegate(void)
		: m_stub()
	{
	}

	/// Binds a free function to the delegate.
	template <R(*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5)>
	void Bind(void)
	{
		m_stub.instance.as_void = nullptr;
		m_stub.function = &FunctionStub<Function>;
	}

	/// Binds a non-const class method to the delegate.
	template <class C, R(C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5)>
	void Bind(NonConstWrapper<C, Function> wrapper)
	{
		m_stub.instance.as_void = wrapper.m_instance;
		m_stub.function = &ClassMethodStub<C, Function>;
	}

	/// Binds a const class method to the delegate.
	template <class C, R(C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5) const>
	void Bind(ConstWrapper<C, Function> wrapper)
	{
		m_stub.instance.as_const_void = wrapper.m_instance;
		m_stub.function = &ConstClassMethodStub<C, Function>;
	}

	/// Invokes the delegate.
	R Invoke(ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5) const
	{
		X_ASSERT(m_stub.function != nullptr, "Cannot invoke unbound delegate. Call Bind() first.")();
		return m_stub.function(m_stub.instance, arg0, arg1, arg2, arg3, arg4, arg5);
	}

private:
	Stub m_stub;
};



/// \brief Partial template specialization of the Delegate base template.
/// \details The template specialization is implemented using the preprocessor library, and built using
/// a custom pre-build step.
/// \sa Delegate
template <typename R, typename ARG0, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6>
class Delegate<R(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6)>
{
	/// Internal union that can hold a pointer to both non-const and const class instances.
	union InstancePtr
	{
		/// Default constructor, initializing to \c nullptr.
		inline InstancePtr(void)
		: as_void(nullptr)
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
	typedef traits::Function<R(InstancePtr, ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6)> InternalFunction;

	/// \brief Internal stub that holds both a class instance (if any) and a pointer to the function which is called
	/// when the delegate is invoked.
	struct Stub
	{
		/// Default constructor, initializing to \c nullptr.
		inline Stub(void)
		: instance()
		, function(nullptr)
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
	template <R(*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6)>
	static X_INLINE R FunctionStub(InstancePtr, ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6)
	{
		// we don't need the instance pointer because we're dealing with free functions
		return (Function)(arg0, arg1, arg2, arg3, arg4, arg5, arg6);
	}

	/// Internal function used when binding delegates to non-const member functions.
	template <class C, R(C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6)>
	static X_INLINE R ClassMethodStub(InstancePtr instance, ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6)
	{
		// cast the instance pointer back into the original class instance. this does not compromise type-safety
		// because we always know the concrete class type, given as a template argument.
		return (static_cast<C*>(instance.as_void)->*Function)(arg0, arg1, arg2, arg3, arg4, arg5, arg6);
	}

	/// Internal function used when binding delegates to const member functions.
	template <class C, R(C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6) const>
	static X_INLINE R ConstClassMethodStub(InstancePtr instance, ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6)
	{
		// cast the instance pointer back into the original class instance. this does not compromise type-safety
		// because we always know the concrete class type, given as a template argument.
		return (static_cast<const C*>(instance.as_const_void)->*Function)(arg0, arg1, arg2, arg3, arg4, arg5, arg6);
	}

public:
	/// Internal wrapper class used as a helper in Bind() overload resolution for non-const member functions.
	template <class C, R(C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6)>
	struct NonConstWrapper
	{
		/// Default constructor.
		NonConstWrapper(C* instance)
		: m_instance(instance)
		{
		}

		C* m_instance;
	};

	/// Internal wrapper class used as a helper in Bind() overload resolution for const member functions.
	template <class C, R(C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6) const>
	struct ConstWrapper
	{
		/// Default constructor.
		ConstWrapper(const C* instance)
		: m_instance(instance)
		{
		}

		const C* m_instance;
	};


	/// Default constructor.
	Delegate(void)
		: m_stub()
	{
	}

	/// Binds a free function to the delegate.
	template <R(*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6)>
	void Bind(void)
	{
		m_stub.instance.as_void = nullptr;
		m_stub.function = &FunctionStub<Function>;
	}

	/// Binds a non-const class method to the delegate.
	template <class C, R(C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6)>
	void Bind(NonConstWrapper<C, Function> wrapper)
	{
		m_stub.instance.as_void = wrapper.m_instance;
		m_stub.function = &ClassMethodStub<C, Function>;
	}

	/// Binds a const class method to the delegate.
	template <class C, R(C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6) const>
	void Bind(ConstWrapper<C, Function> wrapper)
	{
		m_stub.instance.as_const_void = wrapper.m_instance;
		m_stub.function = &ConstClassMethodStub<C, Function>;
	}

	/// Invokes the delegate.
	R Invoke(ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6) const
	{
		X_ASSERT(m_stub.function != nullptr, "Cannot invoke unbound delegate. Call Bind() first.")();
		return m_stub.function(m_stub.instance, arg0, arg1, arg2, arg3, arg4, arg5, arg6);
	}

private:
	Stub m_stub;
};



/// \brief Partial template specialization of the Delegate base template.
/// \details The template specialization is implemented using the preprocessor library, and built using
/// a custom pre-build step.
/// \sa Delegate
template <typename R, typename ARG0, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6, typename ARG7>
class Delegate<R(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7)>
{
	/// Internal union that can hold a pointer to both non-const and const class instances.
	union InstancePtr
	{
		/// Default constructor, initializing to \c nullptr.
		inline InstancePtr(void)
		: as_void(nullptr)
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
	typedef traits::Function<R(InstancePtr, ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7)> InternalFunction;

	/// \brief Internal stub that holds both a class instance (if any) and a pointer to the function which is called
	/// when the delegate is invoked.
	struct Stub
	{
		/// Default constructor, initializing to \c nullptr.
		inline Stub(void)
		: instance()
		, function(nullptr)
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
	template <R(*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7)>
	static X_INLINE R FunctionStub(InstancePtr, ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6, ARG7 arg7)
	{
		// we don't need the instance pointer because we're dealing with free functions
		return (Function)(arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7);
	}

	/// Internal function used when binding delegates to non-const member functions.
	template <class C, R(C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7)>
	static X_INLINE R ClassMethodStub(InstancePtr instance, ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6, ARG7 arg7)
	{
		// cast the instance pointer back into the original class instance. this does not compromise type-safety
		// because we always know the concrete class type, given as a template argument.
		return (static_cast<C*>(instance.as_void)->*Function)(arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7);
	}

	/// Internal function used when binding delegates to const member functions.
	template <class C, R(C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7) const>
	static X_INLINE R ConstClassMethodStub(InstancePtr instance, ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6, ARG7 arg7)
	{
		// cast the instance pointer back into the original class instance. this does not compromise type-safety
		// because we always know the concrete class type, given as a template argument.
		return (static_cast<const C*>(instance.as_const_void)->*Function)(arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7);
	}

public:
	/// Internal wrapper class used as a helper in Bind() overload resolution for non-const member functions.
	template <class C, R(C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7)>
	struct NonConstWrapper
	{
		/// Default constructor.
		NonConstWrapper(C* instance)
		: m_instance(instance)
		{
		}

		C* m_instance;
	};

	/// Internal wrapper class used as a helper in Bind() overload resolution for const member functions.
	template <class C, R(C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7) const>
	struct ConstWrapper
	{
		/// Default constructor.
		ConstWrapper(const C* instance)
		: m_instance(instance)
		{
		}

		const C* m_instance;
	};


	/// Default constructor.
	Delegate(void)
		: m_stub()
	{
	}

	/// Binds a free function to the delegate.
	template <R(*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7)>
	void Bind(void)
	{
		m_stub.instance.as_void = nullptr;
		m_stub.function = &FunctionStub<Function>;
	}

	/// Binds a non-const class method to the delegate.
	template <class C, R(C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7)>
	void Bind(NonConstWrapper<C, Function> wrapper)
	{
		m_stub.instance.as_void = wrapper.m_instance;
		m_stub.function = &ClassMethodStub<C, Function>;
	}

	/// Binds a const class method to the delegate.
	template <class C, R(C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7) const>
	void Bind(ConstWrapper<C, Function> wrapper)
	{
		m_stub.instance.as_const_void = wrapper.m_instance;
		m_stub.function = &ConstClassMethodStub<C, Function>;
	}

	/// Invokes the delegate.
	R Invoke(ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6, ARG7 arg7) const
	{
		X_ASSERT(m_stub.function != nullptr, "Cannot invoke unbound delegate. Call Bind() first.")();
		return m_stub.function(m_stub.instance, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7);
	}

private:
	Stub m_stub;
};




X_NAMESPACE_END



