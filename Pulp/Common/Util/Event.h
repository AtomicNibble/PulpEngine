#pragma once



X_NAMESPACE_BEGIN(core)


template <typename T>
class Event {};



/// \brief Partial template specialization of the Event base template.
/// \details The template specialization is implemented using the preprocessor library, and built using
/// a custom pre-build step.
/// \sa Event
template <typename R  >
class Event<R ()>
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
	typedef traits::Function<R (InstancePtr  )> InternalFunction;

	/// \brief Internal stub that holds both a class instance (if any) and a pointer to the function which is called
	/// when the event is signaled.
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

	/// Internal function used when binding sinks to free functions.
	template <R (*Function)()>
	static X_INLINE R FunctionStub(InstancePtr  )
	{
		// we don't need the instance pointer because we're dealing with free functions
		return (Function)();
	}

	/// Internal function used when binding sinks to non-const member functions.
	template <class C, R (C::*Function)()>
	static X_INLINE R ClassMethodStub(InstancePtr instance  )
	{
		// cast the instance pointer back into the original class instance. this does not compromise type-safety
		// because we always know the concrete class type, given as a template argument.
		return (static_cast<C*>(instance.as_void)->*Function)();
	}

	/// Internal function used when binding sinks to const member functions.
	template <class C, R (C::*Function)() const>
	static X_INLINE R ConstClassMethodStub(InstancePtr instance  )
	{
		// cast the instance pointer back into the original class instance. this does not compromise type-safety
		// because we always know the concrete class type, given as a template argument.
		return (static_cast<const C*>(instance.as_const_void)->*Function)();
	}

public:
	/// \brief A class responsible for adding/removing listeners which are to be signaled by an Event.
	class Sink
	{
	public:
		/// Internal wrapper class used as a helper in AddListener()/RemoveListener() overload resolution for non-const member functions.
		template <class C, R (C::*Function)()>
		struct NonConstWrapper
		{
			/// Default constructor.
			NonConstWrapper(C* instance)
				: instance_(instance)
			{
			}

			C* instance_;
		};

		/// Internal wrapper class used as a helper in AddListener()/RemoveListener() overload resolution for const member functions.
		template <class C, R (C::*Function)() const>
		struct ConstWrapper
		{
			/// Default constructor.
			ConstWrapper(const C* instance)
				: instance_(instance)
			{
			}

			const C* instance_;
		};


		/// \brief Constructs a sink, allocating memory to store the given number of listeners in an array.
		/// \sa Array
		Sink(MemoryArenaBase* arena, size_t numListeners)
			: listeners_(arena)
		{
			listeners_.Reserve(numListeners);
		}

		/// Adds a free function as listener.
		template <R (*Function)()>
		void AddListener(void)
		{
			Stub stub;
			stub.instance.as_void = nullptr;
			stub.function = &FunctionStub<Function>;

			listeners_.Add(stub);
		}

		/// Adds a non-const class method as listener.
		template <class C, R (C::*Function)()>
		void AddListener(NonConstWrapper<C, Function> wrapper)
		{
			Stub stub;
			stub.instance.as_void = wrapper.instance_;
			stub.function = &ClassMethodStub<C, Function>;

			listeners_.Add(stub);
		}

		/// Adds a const class method as listener.
		template <class C, R (C::*Function)() const>
		void AddListener(ConstWrapper<C, Function> wrapper)
		{
			Stub stub;
			stub.instance.as_const_void = wrapper.instance_;
			stub.function = &ConstClassMethodStub<C, Function>;

			listeners_.Add(stub);
		}

		/// \brief Removes a free function as listener.
		/// \remark The order of listeners inside the sink is not preserved.
		template <R (*Function)()>
		void RemoveListener(void)
		{
			Stub stub;
			stub.instance.as_void = nullptr;
			stub.function = &FunctionStub<Function>;

			listeners_.RemoveContiguous(stub);
		}

		/// \brief Removes a non-const class method as listener.
		/// \remark The order of listeners inside the sink is not preserved.
		template <class C, R (C::*Function)()>
		void RemoveListener(NonConstWrapper<C, Function> wrapper)
		{
			Stub stub;
			stub.instance.as_void = wrapper.instance_;
			stub.function = &ClassMethodStub<C, Function>;

			listeners_.RemoveContiguous(stub);
		}

		/// \brief Removes a const class method as listener.
		/// \remark The order of listeners inside the sink is not preserved.
		template <class C, R (C::*Function)() const>
		void RemoveListener(ConstWrapper<C, Function> wrapper)
		{
			Stub stub;
			stub.instance.as_const_void = wrapper.instance_;
			stub.function = &ConstClassMethodStub<C, Function>;

			listeners_.RemoveContiguous(stub);
		}

		/// Returns the number of listeners.
		inline size_t GetListenerCount(void) const
		{
			return listeners_.Num();
		}

		/// Returns the i-th listener.
		inline const Stub& GetListener(size_t i) const
		{
			return listeners_[i];
		}

	private:
		X_NO_COPY(Sink);
		X_NO_ASSIGN(Sink);

		xArray<Stub> listeners_;
	};


	/// Default constructor.
	Event(void)
		: sink_(nullptr)
	{
	}

	/// Binds a sink to the event.
	inline void Bind(Sink* sink)
	{
		sink_ = sink;
	}

	/// Signals the sink.
	void Signal() const
	{
		X_ASSERT(sink_ != nullptr, "Cannot signal unbound event. Call Bind() first.")();

		for (size_t i=0; i<sink_->GetListenerCount(); ++i)
		{
			const Stub& stub = sink_->GetListener(i);
			stub.function(stub.instance  );
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




/// \brief Partial template specialization of the Event base template.
/// \details The template specialization is implemented using the preprocessor library, and built using
/// a custom pre-build step.
/// \sa Event
template <typename R , typename ARG0>
class Event<R (ARG0)>
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
	typedef traits::Function<R (InstancePtr , ARG0)> InternalFunction;

	/// \brief Internal stub that holds both a class instance (if any) and a pointer to the function which is called
	/// when the event is signaled.
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

	/// Internal function used when binding sinks to free functions.
	template <R (*Function)(ARG0)>
	static X_INLINE R FunctionStub(InstancePtr , ARG0 arg0)
	{
		// we don't need the instance pointer because we're dealing with free functions
		return (Function)(arg0);
	}

	/// Internal function used when binding sinks to non-const member functions.
	template <class C, R (C::*Function)(ARG0)>
	static X_INLINE R ClassMethodStub(InstancePtr instance , ARG0 arg0)
	{
		// cast the instance pointer back into the original class instance. this does not compromise type-safety
		// because we always know the concrete class type, given as a template argument.
		return (static_cast<C*>(instance.as_void)->*Function)(arg0);
	}

	/// Internal function used when binding sinks to const member functions.
	template <class C, R (C::*Function)(ARG0) const>
	static X_INLINE R ConstClassMethodStub(InstancePtr instance , ARG0 arg0)
	{
		// cast the instance pointer back into the original class instance. this does not compromise type-safety
		// because we always know the concrete class type, given as a template argument.
		return (static_cast<const C*>(instance.as_const_void)->*Function)(arg0);
	}

public:
	/// \brief A class responsible for adding/removing listeners which are to be signaled by an Event.
	class Sink
	{
	public:


		/// Internal wrapper class used as a helper in AddListener()/RemoveListener() overload resolution for non-const member functions.
		template <class C, R (C::*Function)(ARG0)>
		struct NonConstWrapper
		{
			/// Default constructor.
			NonConstWrapper(C* instance)
				: instance_(instance)
			{
			}

			C* instance_;
		};

		// ned to check if hte pointer is const.
		/// Internal wrapper class used as a helper in AddListener()/RemoveListener() overload resolution for const member functions.
		template <class C, R(C::*Function)(ARG0) const >
		struct ConstWrapper
		{
			/// Default constructor.
			ConstWrapper(const C* instance)
				: instance_(instance)
			{
			}

			const C* instance_;
		};


		/// \brief Constructs a sink, allocating memory to store the given number of listeners in an array.
		/// \sa Array
		Sink(MemoryArenaBase* arena, size_t numListeners)
			: listeners_(arena)
		{
			listeners_.Reserve(numListeners);
		}

		/// Adds a free function as listener.
		template <R (*Function)(ARG0)>
		void AddListener(void)
		{
			Stub stub;
			stub.instance.as_void = nullptr;
			stub.function = &FunctionStub<Function>;

			listeners_.Add(stub);
		}

		/// Adds a non-const class method as listener.
		template <class C, R (C::*Function)(ARG0)>
		void AddListener(NonConstWrapper<C, Function> wrapper)
		{
			Stub stub;
			stub.instance.as_void = wrapper.instance_;
			stub.function = &ClassMethodStub<C, Function>;

			listeners_.Add(stub);
		}

		/// Adds a const class method as listener.
	/*	template <class C, R (C::*Function)(ARG0) const>
		void AddListener(ConstWrapper<C, Function> wrapper)
		{
			Stub stub;
			stub.instance.as_const_void = wrapper.instance_;
			stub.function = &ConstClassMethodStub<C, Function>;

			listeners_.Add(stub);
		}*/

		/// \brief Removes a free function as listener.
		/// \remark The order of listeners inside the sink is not preserved.
		template <R (*Function)(ARG0)>
		void RemoveListener(void)
		{
			Stub stub;
			stub.instance.as_void = nullptr;
			stub.function = &FunctionStub<Function>;

			listeners_.RemoveContiguous(stub);
		}

		/// \brief Removes a non-const class method as listener.
		/// \remark The order of listeners inside the sink is not preserved.
		template <class C, R (C::*Function)(ARG0)>
		void RemoveListener(NonConstWrapper<C, Function> wrapper)
		{
			Stub stub;
			stub.instance.as_void = wrapper.instance_;
			stub.function = &ClassMethodStub<C, Function>;

			listeners_.RemoveContiguous(stub);
		}

		/// \brief Removes a const class method as listener.
		/// \remark The order of listeners inside the sink is not preserved.
		template <class C, R (C::*Function)(ARG0) const>
		void RemoveListener(ConstWrapper<C, Function> wrapper)
		{
			Stub stub;
			stub.instance.as_const_void = wrapper.instance_;
			stub.function = &ConstClassMethodStub<C, Function>;

			listeners_.RemoveContiguous(stub);
		}

		/// Returns the number of listeners.
		inline size_t GetListenerCount(void) const
		{
			return listeners_.Num();
		}

		/// Returns the i-th listener.
		inline const Stub& GetListener(size_t i) const
		{
			return listeners_[i];
		}

	private:
		X_NO_COPY(Sink);
		X_NO_ASSIGN(Sink);

		xArray<Stub> listeners_;
	};


	/// Default constructor.
	Event(void)
		: sink_(nullptr)
	{
	}

	/// Binds a sink to the event.
	inline void Bind(Sink* sink)
	{
		sink_ = sink;
	}

	/// Signals the sink.
	void Signal(ARG0 arg0) const
	{
		X_ASSERT(sink_ != nullptr, "Cannot signal unbound event. Call Bind() first.")();

		for (size_t i=0; i<sink_->GetListenerCount(); ++i)
		{
			const Stub& stub = sink_->GetListener(i);
			stub.function(stub.instance , arg0);
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





/// \brief Partial template specialization of the Event base template.
/// \details The template specialization is implemented using the preprocessor library, and built using
/// a custom pre-build step.
/// \sa Event
template <typename R , typename ARG0, typename ARG1>
class Event<R (ARG0, ARG1)>
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
	typedef traits::Function<R (InstancePtr , ARG0, ARG1)> InternalFunction;

	/// \brief Internal stub that holds both a class instance (if any) and a pointer to the function which is called
	/// when the event is signaled.
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

	/// Internal function used when binding sinks to free functions.
	template <R (*Function)(ARG0, ARG1)>
	static X_INLINE R FunctionStub(InstancePtr , ARG0 arg0, ARG1 arg1)
	{
		// we don't need the instance pointer because we're dealing with free functions
		return (Function)(arg0, arg1);
	}

	/// Internal function used when binding sinks to non-const member functions.
	template <class C, R (C::*Function)(ARG0, ARG1)>
	static X_INLINE R ClassMethodStub(InstancePtr instance , ARG0 arg0, ARG1 arg1)
	{
		// cast the instance pointer back into the original class instance. this does not compromise type-safety
		// because we always know the concrete class type, given as a template argument.
		return (static_cast<C*>(instance.as_void)->*Function)(arg0, arg1);
	}

	/// Internal function used when binding sinks to const member functions.
	template <class C, R (C::*Function)(ARG0, ARG1) const>
	static X_INLINE R ConstClassMethodStub(InstancePtr instance , ARG0 arg0, ARG1 arg1)
	{
		// cast the instance pointer back into the original class instance. this does not compromise type-safety
		// because we always know the concrete class type, given as a template argument.
		return (static_cast<const C*>(instance.as_const_void)->*Function)(arg0, arg1);
	}

public:
	/// \brief A class responsible for adding/removing listeners which are to be signaled by an Event.
	class Sink
	{
	public:
		/// Internal wrapper class used as a helper in AddListener()/RemoveListener() overload resolution for non-const member functions.
		template <class C, R (C::*Function)(ARG0, ARG1)>
		struct NonConstWrapper
		{
			/// Default constructor.
			NonConstWrapper(C* instance)
				: instance_(instance)
			{
			}

			C* instance_;
		};

		/// Internal wrapper class used as a helper in AddListener()/RemoveListener() overload resolution for const member functions.
		template <class C, R (C::*Function)(ARG0, ARG1) const>
		struct ConstWrapper
		{
			/// Default constructor.
			ConstWrapper(const C* instance)
				: instance_(instance)
			{
			}

			const C* instance_;
		};


		/// \brief Constructs a sink, allocating memory to store the given number of listeners in an array.
		/// \sa Array
		Sink(MemoryArenaBase* arena, size_t numListeners)
			: listeners_(arena)
		{
			listeners_.Reserve(numListeners);
		}

		/// Adds a free function as listener.
		template <R (*Function)(ARG0, ARG1)>
		void AddListener(void)
		{
			Stub stub;
			stub.instance.as_void = nullptr;
			stub.function = &FunctionStub<Function>;

			listeners_.Add(stub);
		}

		/// Adds a non-const class method as listener.
		template <class C, R (C::*Function)(ARG0, ARG1)>
		void AddListener(NonConstWrapper<C, Function> wrapper)
		{
			Stub stub;
			stub.instance.as_void = wrapper.instance_;
			stub.function = &ClassMethodStub<C, Function>;

			listeners_.Add(stub);
		}

		/// Adds a const class method as listener.
		template <class C, R (C::*Function)(ARG0, ARG1) const>
		void AddListener(ConstWrapper<C, Function> wrapper)
		{
			Stub stub;
			stub.instance.as_const_void = wrapper.instance_;
			stub.function = &ConstClassMethodStub<C, Function>;

			listeners_.Add(stub);
		}

		/// \brief Removes a free function as listener.
		/// \remark The order of listeners inside the sink is not preserved.
		template <R (*Function)(ARG0, ARG1)>
		void RemoveListener(void)
		{
			Stub stub;
			stub.instance.as_void = nullptr;
			stub.function = &FunctionStub<Function>;

			listeners_.RemoveContiguous(stub);
		}

		/// \brief Removes a non-const class method as listener.
		/// \remark The order of listeners inside the sink is not preserved.
		template <class C, R (C::*Function)(ARG0, ARG1)>
		void RemoveListener(NonConstWrapper<C, Function> wrapper)
		{
			Stub stub;
			stub.instance.as_void = wrapper.instance_;
			stub.function = &ClassMethodStub<C, Function>;

			listeners_.RemoveContiguous(stub);
		}

		/// \brief Removes a const class method as listener.
		/// \remark The order of listeners inside the sink is not preserved.
		template <class C, R (C::*Function)(ARG0, ARG1) const>
		void RemoveListener(ConstWrapper<C, Function> wrapper)
		{
			Stub stub;
			stub.instance.as_const_void = wrapper.instance_;
			stub.function = &ConstClassMethodStub<C, Function>;

			listeners_.RemoveContiguous(stub);
		}

		/// Returns the number of listeners.
		inline size_t GetListenerCount(void) const
		{
			return listeners_.Num();
		}

		/// Returns the i-th listener.
		inline const Stub& GetListener(size_t i) const
		{
			return listeners_[i];
		}

	private:
		X_NO_COPY(Sink);
		X_NO_ASSIGN(Sink);

		xArray<Stub> listeners_;
	};


	/// Default constructor.
	Event(void)
		: sink_(nullptr)
	{
	}

	/// Binds a sink to the event.
	inline void Bind(Sink* sink)
	{
		sink_ = sink;
	}

	/// Signals the sink.
	void Signal(ARG0 arg0, ARG1 arg1) const
	{
		X_ASSERT(sink_ != nullptr, "Cannot signal unbound event. Call Bind() first.")();

		for (size_t i=0; i<sink_->GetListenerCount(); ++i)
		{
			const Stub& stub = sink_->GetListener(i);
			stub.function(stub.instance , arg0, arg1);
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




/// \brief Partial template specialization of the Event base template.
/// \details The template specialization is implemented using the preprocessor library, and built using
/// a custom pre-build step.
/// \sa Event
template <typename R , typename ARG0, typename ARG1, typename ARG2>
class Event<R (ARG0, ARG1, ARG2)>
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
	typedef traits::Function<R (InstancePtr , ARG0, ARG1, ARG2)> InternalFunction;

	/// \brief Internal stub that holds both a class instance (if any) and a pointer to the function which is called
	/// when the event is signaled.
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

	/// Internal function used when binding sinks to free functions.
	template <R (*Function)(ARG0, ARG1, ARG2)>
	static X_INLINE R FunctionStub(InstancePtr , ARG0 arg0, ARG1 arg1, ARG2 arg2)
	{
		// we don't need the instance pointer because we're dealing with free functions
		return (Function)(arg0, arg1, arg2);
	}

	/// Internal function used when binding sinks to non-const member functions.
	template <class C, R (C::*Function)(ARG0, ARG1, ARG2)>
	static X_INLINE R ClassMethodStub(InstancePtr instance , ARG0 arg0, ARG1 arg1, ARG2 arg2)
	{
		// cast the instance pointer back into the original class instance. this does not compromise type-safety
		// because we always know the concrete class type, given as a template argument.
		return (static_cast<C*>(instance.as_void)->*Function)(arg0, arg1, arg2);
	}

	/// Internal function used when binding sinks to const member functions.
	template <class C, R (C::*Function)(ARG0, ARG1, ARG2) const>
	static X_INLINE R ConstClassMethodStub(InstancePtr instance , ARG0 arg0, ARG1 arg1, ARG2 arg2)
	{
		// cast the instance pointer back into the original class instance. this does not compromise type-safety
		// because we always know the concrete class type, given as a template argument.
		return (static_cast<const C*>(instance.as_const_void)->*Function)(arg0, arg1, arg2);
	}

public:
	/// \brief A class responsible for adding/removing listeners which are to be signaled by an Event.
	class Sink
	{
	public:
		/// Internal wrapper class used as a helper in AddListener()/RemoveListener() overload resolution for non-const member functions.
		template <class C, R (C::*Function)(ARG0, ARG1, ARG2)>
		struct NonConstWrapper
		{
			/// Default constructor.
			NonConstWrapper(C* instance)
				: instance_(instance)
			{
			}

			C* instance_;
		};

		/// Internal wrapper class used as a helper in AddListener()/RemoveListener() overload resolution for const member functions.
		template <class C, R (C::*Function)(ARG0, ARG1, ARG2) const>
		struct ConstWrapper
		{
			/// Default constructor.
			ConstWrapper(const C* instance)
				: instance_(instance)
			{
			}

			const C* instance_;
		};


		/// \brief Constructs a sink, allocating memory to store the given number of listeners in an array.
		/// \sa Array
		Sink(MemoryArenaBase* arena, size_t numListeners)
			: listeners_(arena)
		{
			listeners_.Reserve(numListeners);
		}

		/// Adds a free function as listener.
		template <R (*Function)(ARG0, ARG1, ARG2)>
		void AddListener(void)
		{
			Stub stub;
			stub.instance.as_void = nullptr;
			stub.function = &FunctionStub<Function>;

			listeners_.Add(stub);
		}

		/// Adds a non-const class method as listener.
		template <class C, R (C::*Function)(ARG0, ARG1, ARG2)>
		void AddListener(NonConstWrapper<C, Function> wrapper)
		{
			Stub stub;
			stub.instance.as_void = wrapper.instance_;
			stub.function = &ClassMethodStub<C, Function>;

			listeners_.Add(stub);
		}

		/// Adds a const class method as listener.
	/*	template <class C, R (C::*Function)(ARG0, ARG1, ARG2) const>
		void AddListener(ConstWrapper<C, Function> wrapper)
		{
			Stub stub;
			stub.instance.as_const_void = wrapper.instance_;
			stub.function = &ConstClassMethodStub<C, Function>;

			listeners_.Add(stub);
		}*/

		/// \brief Removes a free function as listener.
		/// \remark The order of listeners inside the sink is not preserved.
		template <R (*Function)(ARG0, ARG1, ARG2)>
		void RemoveListener(void)
		{
			Stub stub;
			stub.instance.as_void = nullptr;
			stub.function = &FunctionStub<Function>;

			listeners_.RemoveContiguous(stub);
		}

		/// \brief Removes a non-const class method as listener.
		/// \remark The order of listeners inside the sink is not preserved.
		template <class C, R (C::*Function)(ARG0, ARG1, ARG2)>
		void RemoveListener(NonConstWrapper<C, Function> wrapper)
		{
			Stub stub;
			stub.instance.as_void = wrapper.instance_;
			stub.function = &ClassMethodStub<C, Function>;

			listeners_.RemoveContiguous(stub);
		}

		/// \brief Removes a const class method as listener.
		/// \remark The order of listeners inside the sink is not preserved.
		template <class C, R (C::*Function)(ARG0, ARG1, ARG2) const>
		void RemoveListener(ConstWrapper<C, Function> wrapper)
		{
			Stub stub;
			stub.instance.as_const_void = wrapper.instance_;
			stub.function = &ConstClassMethodStub<C, Function>;

			listeners_.RemoveContiguous(stub);
		}

		/// Returns the number of listeners.
		inline size_t GetListenerCount(void) const
		{
			return listeners_.Num();
		}

		/// Returns the i-th listener.
		inline const Stub& GetListener(size_t i) const
		{
			return listeners_[i];
		}

	private:
		X_NO_COPY(Sink);
		X_NO_ASSIGN(Sink);

		xArray<Stub> listeners_;
	};


	/// Default constructor.
	Event(void)
		: sink_(nullptr)
	{
	}

	/// Binds a sink to the event.
	inline void Bind(Sink* sink)
	{
		sink_ = sink;
	}

	/// Signals the sink.
	void Signal(ARG0 arg0, ARG1 arg1, ARG2 arg2) const
	{
		X_ASSERT(sink_ != nullptr, "Cannot signal unbound event. Call Bind() first.")();

		for (size_t i=0; i<sink_->GetListenerCount(); ++i)
		{
			const Stub& stub = sink_->GetListener(i);
			stub.function(stub.instance , arg0, arg1, arg2);
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










/// \brief Partial template specialization of the Event base template.
/// \details The template specialization is implemented using the preprocessor library, and built using
/// a custom pre-build step.
/// \sa Event
template <typename R , typename ARG0, typename ARG1, typename ARG2, typename ARG3>
class Event<R (ARG0, ARG1, ARG2, ARG3)>
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
	typedef traits::Function<R (InstancePtr , ARG0, ARG1, ARG2, ARG3)> InternalFunction;

	/// \brief Internal stub that holds both a class instance (if any) and a pointer to the function which is called
	/// when the event is signaled.
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

	/// Internal function used when binding sinks to free functions.
	template <R (*Function)(ARG0, ARG1, ARG2, ARG3)>
	static X_INLINE R FunctionStub(InstancePtr , ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3)
	{
		// we don't need the instance pointer because we're dealing with free functions
		return (Function)(arg0, arg1, arg2, arg3);
	}

	/// Internal function used when binding sinks to non-const member functions.
	template <class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3)>
	static X_INLINE R ClassMethodStub(InstancePtr instance , ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3)
	{
		// cast the instance pointer back into the original class instance. this does not compromise type-safety
		// because we always know the concrete class type, given as a template argument.
		return (static_cast<C*>(instance.as_void)->*Function)(arg0, arg1, arg2, arg3);
	}

	/// Internal function used when binding sinks to const member functions.
	template <class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3) const>
	static X_INLINE R ConstClassMethodStub(InstancePtr instance , ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3)
	{
		// cast the instance pointer back into the original class instance. this does not compromise type-safety
		// because we always know the concrete class type, given as a template argument.
		return (static_cast<const C*>(instance.as_const_void)->*Function)(arg0, arg1, arg2, arg3);
	}

public:
	/// \brief A class responsible for adding/removing listeners which are to be signaled by an Event.
	class Sink
	{
	public:
		/// Internal wrapper class used as a helper in AddListener()/RemoveListener() overload resolution for non-const member functions.
		template <class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3)>
		struct NonConstWrapper
		{
			/// Default constructor.
			NonConstWrapper(C* instance)
				: instance_(instance)
			{
			}

			C* instance_;
		};

		/// Internal wrapper class used as a helper in AddListener()/RemoveListener() overload resolution for const member functions.
		template <class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3) const>
		struct ConstWrapper
		{
			/// Default constructor.
			ConstWrapper(const C* instance)
				: instance_(instance)
			{
			}

			const C* instance_;
		};


		/// \brief Constructs a sink, allocating memory to store the given number of listeners in an array.
		/// \sa Array
		Sink(MemoryArenaBase* arena, size_t numListeners)
			: listeners_(arena)
		{
			listeners_.Reserve(numListeners);
		}

		/// Adds a free function as listener.
		template <R (*Function)(ARG0, ARG1, ARG2, ARG3)>
		void AddListener(void)
		{
			Stub stub;
			stub.instance.as_void = nullptr;
			stub.function = &FunctionStub<Function>;

			listeners_.Add(stub);
		}

		/// Adds a non-const class method as listener.
		template <class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3)>
		void AddListener(NonConstWrapper<C, Function> wrapper)
		{
			Stub stub;
			stub.instance.as_void = wrapper.instance_;
			stub.function = &ClassMethodStub<C, Function>;

			listeners_.Add(stub);
		}

		/// Adds a const class method as listener.
		template <class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3) const>
		void AddListener(ConstWrapper<C, Function> wrapper)
		{
			Stub stub;
			stub.instance.as_const_void = wrapper.instance_;
			stub.function = &ConstClassMethodStub<C, Function>;

			listeners_.Add(stub);
		}

		/// \brief Removes a free function as listener.
		/// \remark The order of listeners inside the sink is not preserved.
		template <R (*Function)(ARG0, ARG1, ARG2, ARG3)>
		void RemoveListener(void)
		{
			Stub stub;
			stub.instance.as_void = nullptr;
			stub.function = &FunctionStub<Function>;

			listeners_.RemoveContiguous(stub);
		}

		/// \brief Removes a non-const class method as listener.
		/// \remark The order of listeners inside the sink is not preserved.
		template <class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3)>
		void RemoveListener(NonConstWrapper<C, Function> wrapper)
		{
			Stub stub;
			stub.instance.as_void = wrapper.instance_;
			stub.function = &ClassMethodStub<C, Function>;

			listeners_.RemoveContiguous(stub);
		}

		/// \brief Removes a const class method as listener.
		/// \remark The order of listeners inside the sink is not preserved.
		template <class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3) const>
		void RemoveListener(ConstWrapper<C, Function> wrapper)
		{
			Stub stub;
			stub.instance.as_const_void = wrapper.instance_;
			stub.function = &ConstClassMethodStub<C, Function>;

			listeners_.RemoveContiguous(stub);
		}

		/// Returns the number of listeners.
		inline size_t GetListenerCount(void) const
		{
			return listeners_.Num();
		}

		/// Returns the i-th listener.
		inline const Stub& GetListener(size_t i) const
		{
			return listeners_[i];
		}

	private:
		X_NO_COPY(Sink);
		X_NO_ASSIGN(Sink);

		xArray<Stub> listeners_;
	};


	/// Default constructor.
	Event(void)
		: sink_(nullptr)
	{
	}

	/// Binds a sink to the event.
	inline void Bind(Sink* sink)
	{
		sink_ = sink;
	}

	/// Signals the sink.
	void Signal(ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3) const
	{
		X_ASSERT(sink_ != nullptr, "Cannot signal unbound event. Call Bind() first.")();

		for (size_t i=0; i<sink_->GetListenerCount(); ++i)
		{
			const Stub& stub = sink_->GetListener(i);
			stub.function(stub.instance , arg0, arg1, arg2, arg3);
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







/// \brief Partial template specialization of the Event base template.
/// \details The template specialization is implemented using the preprocessor library, and built using
/// a custom pre-build step.
/// \sa Event
template <typename R , typename ARG0, typename ARG1, typename ARG2, typename ARG3, typename ARG4>
class Event<R (ARG0, ARG1, ARG2, ARG3, ARG4)>
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
	typedef traits::Function<R (InstancePtr , ARG0, ARG1, ARG2, ARG3, ARG4)> InternalFunction;

	/// \brief Internal stub that holds both a class instance (if any) and a pointer to the function which is called
	/// when the event is signaled.
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

	/// Internal function used when binding sinks to free functions.
	template <R (*Function)(ARG0, ARG1, ARG2, ARG3, ARG4)>
	static X_INLINE R FunctionStub(InstancePtr , ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4)
	{
		// we don't need the instance pointer because we're dealing with free functions
		return (Function)(arg0, arg1, arg2, arg3, arg4);
	}

	/// Internal function used when binding sinks to non-const member functions.
	template <class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4)>
	static X_INLINE R ClassMethodStub(InstancePtr instance , ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4)
	{
		// cast the instance pointer back into the original class instance. this does not compromise type-safety
		// because we always know the concrete class type, given as a template argument.
		return (static_cast<C*>(instance.as_void)->*Function)(arg0, arg1, arg2, arg3, arg4);
	}

	/// Internal function used when binding sinks to const member functions.
	template <class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4) const>
	static X_INLINE R ConstClassMethodStub(InstancePtr instance , ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4)
	{
		// cast the instance pointer back into the original class instance. this does not compromise type-safety
		// because we always know the concrete class type, given as a template argument.
		return (static_cast<const C*>(instance.as_const_void)->*Function)(arg0, arg1, arg2, arg3, arg4);
	}

public:
	/// \brief A class responsible for adding/removing listeners which are to be signaled by an Event.
	class Sink
	{
	public:
		/// Internal wrapper class used as a helper in AddListener()/RemoveListener() overload resolution for non-const member functions.
		template <class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4)>
		struct NonConstWrapper
		{
			/// Default constructor.
			NonConstWrapper(C* instance)
				: instance_(instance)
			{
			}

			C* instance_;
		};

		/// Internal wrapper class used as a helper in AddListener()/RemoveListener() overload resolution for const member functions.
		template <class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4) const>
		struct ConstWrapper
		{
			/// Default constructor.
			ConstWrapper(const C* instance)
				: instance_(instance)
			{
			}

			const C* instance_;
		};


		/// \brief Constructs a sink, allocating memory to store the given number of listeners in an array.
		/// \sa Array
		Sink(MemoryArenaBase* arena, size_t numListeners)
			: listeners_(arena)
		{
			listeners_.Reserve(numListeners);
		}

		/// Adds a free function as listener.
		template <R (*Function)(ARG0, ARG1, ARG2, ARG3, ARG4)>
		void AddListener(void)
		{
			Stub stub;
			stub.instance.as_void = nullptr;
			stub.function = &FunctionStub<Function>;

			listeners_.Add(stub);
		}

		/// Adds a non-const class method as listener.
		template <class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4)>
		void AddListener(NonConstWrapper<C, Function> wrapper)
		{
			Stub stub;
			stub.instance.as_void = wrapper.instance_;
			stub.function = &ClassMethodStub<C, Function>;

			listeners_.Add(stub);
		}

		/// Adds a const class method as listener.
		template <class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4) const>
		void AddListener(ConstWrapper<C, Function> wrapper)
		{
			Stub stub;
			stub.instance.as_const_void = wrapper.instance_;
			stub.function = &ConstClassMethodStub<C, Function>;

			listeners_.Add(stub);
		}

		/// \brief Removes a free function as listener.
		/// \remark The order of listeners inside the sink is not preserved.
		template <R (*Function)(ARG0, ARG1, ARG2, ARG3, ARG4)>
		void RemoveListener(void)
		{
			Stub stub;
			stub.instance.as_void = nullptr;
			stub.function = &FunctionStub<Function>;

			listeners_.RemoveContiguous(stub);
		}

		/// \brief Removes a non-const class method as listener.
		/// \remark The order of listeners inside the sink is not preserved.
		template <class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4)>
		void RemoveListener(NonConstWrapper<C, Function> wrapper)
		{
			Stub stub;
			stub.instance.as_void = wrapper.instance_;
			stub.function = &ClassMethodStub<C, Function>;

			listeners_.RemoveContiguous(stub);
		}

		/// \brief Removes a const class method as listener.
		/// \remark The order of listeners inside the sink is not preserved.
		template <class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4) const>
		void RemoveListener(ConstWrapper<C, Function> wrapper)
		{
			Stub stub;
			stub.instance.as_const_void = wrapper.instance_;
			stub.function = &ConstClassMethodStub<C, Function>;

			listeners_.RemoveContiguous(stub);
		}

		/// Returns the number of listeners.
		inline size_t GetListenerCount(void) const
		{
			return listeners_.Num();
		}

		/// Returns the i-th listener.
		inline const Stub& GetListener(size_t i) const
		{
			return listeners_[i];
		}

	private:
		X_NO_COPY(Sink);
		X_NO_ASSIGN(Sink);

		xArray<Stub> listeners_;
	};


	/// Default constructor.
	Event(void)
		: sink_(nullptr)
	{
	}

	/// Binds a sink to the event.
	inline void Bind(Sink* sink)
	{
		sink_ = sink;
	}

	/// Signals the sink.
	void Signal(ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4) const
	{
		X_ASSERT(sink_ != nullptr, "Cannot signal unbound event. Call Bind() first.")();

		for (size_t i=0; i<sink_->GetListenerCount(); ++i)
		{
			const Stub& stub = sink_->GetListener(i);
			stub.function(stub.instance , arg0, arg1, arg2, arg3, arg4);
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







/// \brief Partial template specialization of the Event base template.
/// \details The template specialization is implemented using the preprocessor library, and built using
/// a custom pre-build step.
/// \sa Event
template <typename R , typename ARG0, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5>
class Event<R (ARG0, ARG1, ARG2, ARG3, ARG4, ARG5)>
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
	typedef traits::Function<R (InstancePtr , ARG0, ARG1, ARG2, ARG3, ARG4, ARG5)> InternalFunction;

	/// \brief Internal stub that holds both a class instance (if any) and a pointer to the function which is called
	/// when the event is signaled.
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

	/// Internal function used when binding sinks to free functions.
	template <R (*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5)>
	static X_INLINE R FunctionStub(InstancePtr , ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5)
	{
		// we don't need the instance pointer because we're dealing with free functions
		return (Function)(arg0, arg1, arg2, arg3, arg4, arg5);
	}

	/// Internal function used when binding sinks to non-const member functions.
	template <class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5)>
	static X_INLINE R ClassMethodStub(InstancePtr instance , ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5)
	{
		// cast the instance pointer back into the original class instance. this does not compromise type-safety
		// because we always know the concrete class type, given as a template argument.
		return (static_cast<C*>(instance.as_void)->*Function)(arg0, arg1, arg2, arg3, arg4, arg5);
	}

	/// Internal function used when binding sinks to const member functions.
	template <class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5) const>
	static X_INLINE R ConstClassMethodStub(InstancePtr instance , ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5)
	{
		// cast the instance pointer back into the original class instance. this does not compromise type-safety
		// because we always know the concrete class type, given as a template argument.
		return (static_cast<const C*>(instance.as_const_void)->*Function)(arg0, arg1, arg2, arg3, arg4, arg5);
	}

public:
	/// \brief A class responsible for adding/removing listeners which are to be signaled by an Event.
	class Sink
	{
	public:
		/// Internal wrapper class used as a helper in AddListener()/RemoveListener() overload resolution for non-const member functions.
		template <class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5)>
		struct NonConstWrapper
		{
			/// Default constructor.
			NonConstWrapper(C* instance)
				: instance_(instance)
			{
			}

			C* instance_;
		};

		/// Internal wrapper class used as a helper in AddListener()/RemoveListener() overload resolution for const member functions.
		template <class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5) const>
		struct ConstWrapper
		{
			/// Default constructor.
			ConstWrapper(const C* instance)
				: instance_(instance)
			{
			}

			const C* instance_;
		};


		/// \brief Constructs a sink, allocating memory to store the given number of listeners in an array.
		/// \sa Array
		Sink(MemoryArenaBase* arena, size_t numListeners)
			: listeners_(arena)
		{
			listeners_.Reserve(numListeners);
		}

		/// Adds a free function as listener.
		template <R (*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5)>
		void AddListener(void)
		{
			Stub stub;
			stub.instance.as_void = nullptr;
			stub.function = &FunctionStub<Function>;

			listeners_.Add(stub);
		}

		/// Adds a non-const class method as listener.
		template <class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5)>
		void AddListener(NonConstWrapper<C, Function> wrapper)
		{
			Stub stub;
			stub.instance.as_void = wrapper.instance_;
			stub.function = &ClassMethodStub<C, Function>;

			listeners_.Add(stub);
		}

		/// Adds a const class method as listener.
		template <class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5) const>
		void AddListener(ConstWrapper<C, Function> wrapper)
		{
			Stub stub;
			stub.instance.as_const_void = wrapper.instance_;
			stub.function = &ConstClassMethodStub<C, Function>;

			listeners_.Add(stub);
		}

		/// \brief Removes a free function as listener.
		/// \remark The order of listeners inside the sink is not preserved.
		template <R (*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5)>
		void RemoveListener(void)
		{
			Stub stub;
			stub.instance.as_void = nullptr;
			stub.function = &FunctionStub<Function>;

			listeners_.RemoveContiguous(stub);
		}

		/// \brief Removes a non-const class method as listener.
		/// \remark The order of listeners inside the sink is not preserved.
		template <class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5)>
		void RemoveListener(NonConstWrapper<C, Function> wrapper)
		{
			Stub stub;
			stub.instance.as_void = wrapper.instance_;
			stub.function = &ClassMethodStub<C, Function>;

			listeners_.RemoveContiguous(stub);
		}

		/// \brief Removes a const class method as listener.
		/// \remark The order of listeners inside the sink is not preserved.
		template <class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5) const>
		void RemoveListener(ConstWrapper<C, Function> wrapper)
		{
			Stub stub;
			stub.instance.as_const_void = wrapper.instance_;
			stub.function = &ConstClassMethodStub<C, Function>;

			listeners_.RemoveContiguous(stub);
		}

		/// Returns the number of listeners.
		inline size_t GetListenerCount(void) const
		{
			return listeners_.Num();
		}

		/// Returns the i-th listener.
		inline const Stub& GetListener(size_t i) const
		{
			return listeners_[i];
		}

	private:
		X_NO_COPY(Sink);
		X_NO_ASSIGN(Sink);

		xArray<Stub> listeners_;
	};


	/// Default constructor.
	Event(void)
		: sink_(nullptr)
	{
	}

	/// Binds a sink to the event.
	inline void Bind(Sink* sink)
	{
		sink_ = sink;
	}

	/// Signals the sink.
	void Signal(ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5) const
	{
		X_ASSERT(sink_ != nullptr, "Cannot signal unbound event. Call Bind() first.")();

		for (size_t i=0; i<sink_->GetListenerCount(); ++i)
		{
			const Stub& stub = sink_->GetListener(i);
			stub.function(stub.instance , arg0, arg1, arg2, arg3, arg4, arg5);
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





/// \brief Partial template specialization of the Event base template.
/// \details The template specialization is implemented using the preprocessor library, and built using
/// a custom pre-build step.
/// \sa Event
template <typename R , typename ARG0, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6>
class Event<R (ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6)>
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
	typedef traits::Function<R (InstancePtr , ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6)> InternalFunction;

	/// \brief Internal stub that holds both a class instance (if any) and a pointer to the function which is called
	/// when the event is signaled.
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

	/// Internal function used when binding sinks to free functions.
	template <R (*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6)>
	static X_INLINE R FunctionStub(InstancePtr , ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6)
	{
		// we don't need the instance pointer because we're dealing with free functions
		return (Function)(arg0, arg1, arg2, arg3, arg4, arg5, arg6);
	}

	/// Internal function used when binding sinks to non-const member functions.
	template <class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6)>
	static X_INLINE R ClassMethodStub(InstancePtr instance , ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6)
	{
		// cast the instance pointer back into the original class instance. this does not compromise type-safety
		// because we always know the concrete class type, given as a template argument.
		return (static_cast<C*>(instance.as_void)->*Function)(arg0, arg1, arg2, arg3, arg4, arg5, arg6);
	}

	/// Internal function used when binding sinks to const member functions.
	template <class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6) const>
	static X_INLINE R ConstClassMethodStub(InstancePtr instance , ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6)
	{
		// cast the instance pointer back into the original class instance. this does not compromise type-safety
		// because we always know the concrete class type, given as a template argument.
		return (static_cast<const C*>(instance.as_const_void)->*Function)(arg0, arg1, arg2, arg3, arg4, arg5, arg6);
	}

public:
	/// \brief A class responsible for adding/removing listeners which are to be signaled by an Event.
	class Sink
	{
	public:
		/// Internal wrapper class used as a helper in AddListener()/RemoveListener() overload resolution for non-const member functions.
		template <class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6)>
		struct NonConstWrapper
		{
			/// Default constructor.
			NonConstWrapper(C* instance)
				: instance_(instance)
			{
			}

			C* instance_;
		};

		/// Internal wrapper class used as a helper in AddListener()/RemoveListener() overload resolution for const member functions.
		template <class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6) const>
		struct ConstWrapper
		{
			/// Default constructor.
			ConstWrapper(const C* instance)
				: instance_(instance)
			{
			}

			const C* instance_;
		};


		/// \brief Constructs a sink, allocating memory to store the given number of listeners in an array.
		/// \sa Array
		Sink(MemoryArenaBase* arena, size_t numListeners)
			: listeners_(arena)
		{
			listeners_.Reserve(numListeners);
		}

		/// Adds a free function as listener.
		template <R (*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6)>
		void AddListener(void)
		{
			Stub stub;
			stub.instance.as_void = nullptr;
			stub.function = &FunctionStub<Function>;

			listeners_.Add(stub);
		}

		/// Adds a non-const class method as listener.
		template <class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6)>
		void AddListener(NonConstWrapper<C, Function> wrapper)
		{
			Stub stub;
			stub.instance.as_void = wrapper.instance_;
			stub.function = &ClassMethodStub<C, Function>;

			listeners_.Add(stub);
		}

		/// Adds a const class method as listener.
		template <class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6) const>
		void AddListener(ConstWrapper<C, Function> wrapper)
		{
			Stub stub;
			stub.instance.as_const_void = wrapper.instance_;
			stub.function = &ConstClassMethodStub<C, Function>;

			listeners_.Add(stub);
		}

		/// \brief Removes a free function as listener.
		/// \remark The order of listeners inside the sink is not preserved.
		template <R (*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6)>
		void RemoveListener(void)
		{
			Stub stub;
			stub.instance.as_void = nullptr;
			stub.function = &FunctionStub<Function>;

			listeners_.RemoveContiguous(stub);
		}

		/// \brief Removes a non-const class method as listener.
		/// \remark The order of listeners inside the sink is not preserved.
		template <class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6)>
		void RemoveListener(NonConstWrapper<C, Function> wrapper)
		{
			Stub stub;
			stub.instance.as_void = wrapper.instance_;
			stub.function = &ClassMethodStub<C, Function>;

			listeners_.RemoveContiguous(stub);
		}

		/// \brief Removes a const class method as listener.
		/// \remark The order of listeners inside the sink is not preserved.
		template <class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6) const>
		void RemoveListener(ConstWrapper<C, Function> wrapper)
		{
			Stub stub;
			stub.instance.as_const_void = wrapper.instance_;
			stub.function = &ConstClassMethodStub<C, Function>;

			listeners_.RemoveContiguous(stub);
		}

		/// Returns the number of listeners.
		inline size_t GetListenerCount(void) const
		{
			return listeners_.Num();
		}

		/// Returns the i-th listener.
		inline const Stub& GetListener(size_t i) const
		{
			return listeners_[i];
		}

	private:
		X_NO_COPY(Sink);
		X_NO_ASSIGN(Sink);

		xArray<Stub> listeners_;
	};


	/// Default constructor.
	Event(void)
		: sink_(nullptr)
	{
	}

	/// Binds a sink to the event.
	inline void Bind(Sink* sink)
	{
		sink_ = sink;
	}

	/// Signals the sink.
	void Signal(ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6) const
	{
		X_ASSERT(sink_ != nullptr, "Cannot signal unbound event. Call Bind() first.")();

		for (size_t i=0; i<sink_->GetListenerCount(); ++i)
		{
			const Stub& stub = sink_->GetListener(i);
			stub.function(stub.instance , arg0, arg1, arg2, arg3, arg4, arg5, arg6);
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





/// \brief Partial template specialization of the Event base template.
/// \details The template specialization is implemented using the preprocessor library, and built using
/// a custom pre-build step.
/// \sa Event
template <typename R , typename ARG0, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6, typename ARG7>
class Event<R (ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7)>
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
	typedef traits::Function<R (InstancePtr , ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7)> InternalFunction;

	/// \brief Internal stub that holds both a class instance (if any) and a pointer to the function which is called
	/// when the event is signaled.
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

	/// Internal function used when binding sinks to free functions.
	template <R (*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7)>
	static X_INLINE R FunctionStub(InstancePtr , ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6, ARG7 arg7)
	{
		// we don't need the instance pointer because we're dealing with free functions
		return (Function)(arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7);
	}

	/// Internal function used when binding sinks to non-const member functions.
	template <class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7)>
	static X_INLINE R ClassMethodStub(InstancePtr instance , ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6, ARG7 arg7)
	{
		// cast the instance pointer back into the original class instance. this does not compromise type-safety
		// because we always know the concrete class type, given as a template argument.
		return (static_cast<C*>(instance.as_void)->*Function)(arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7);
	}

	/// Internal function used when binding sinks to const member functions.
	template <class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7) const>
	static X_INLINE R ConstClassMethodStub(InstancePtr instance , ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6, ARG7 arg7)
	{
		// cast the instance pointer back into the original class instance. this does not compromise type-safety
		// because we always know the concrete class type, given as a template argument.
		return (static_cast<const C*>(instance.as_const_void)->*Function)(arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7);
	}

public:
	/// \brief A class responsible for adding/removing listeners which are to be signaled by an Event.
	class Sink
	{
	public:
		/// Internal wrapper class used as a helper in AddListener()/RemoveListener() overload resolution for non-const member functions.
		template <class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7)>
		struct NonConstWrapper
		{
			/// Default constructor.
			NonConstWrapper(C* instance)
				: instance_(instance)
			{
			}

			C* instance_;
		};

		/// Internal wrapper class used as a helper in AddListener()/RemoveListener() overload resolution for const member functions.
		template <class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7) const>
		struct ConstWrapper
		{
			/// Default constructor.
			ConstWrapper(const C* instance)
				: instance_(instance)
			{
			}

			const C* instance_;
		};


		/// \brief Constructs a sink, allocating memory to store the given number of listeners in an array.
		/// \sa Array
		Sink(MemoryArenaBase* arena, size_t numListeners)
			: listeners_(arena)
		{
			listeners_.Reserve(numListeners);
		}

		/// Adds a free function as listener.
		template <R (*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7)>
		void AddListener(void)
		{
			Stub stub;
			stub.instance.as_void = nullptr;
			stub.function = &FunctionStub<Function>;

			listeners_.Add(stub);
		}

		/// Adds a non-const class method as listener.
		template <class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7)>
		void AddListener(NonConstWrapper<C, Function> wrapper)
		{
			Stub stub;
			stub.instance.as_void = wrapper.instance_;
			stub.function = &ClassMethodStub<C, Function>;

			listeners_.Add(stub);
		}

		/// Adds a const class method as listener.
		template <class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7) const>
		void AddListener(ConstWrapper<C, Function> wrapper)
		{
			Stub stub;
			stub.instance.as_const_void = wrapper.instance_;
			stub.function = &ConstClassMethodStub<C, Function>;

			listeners_.Add(stub);
		}

		/// \brief Removes a free function as listener.
		/// \remark The order of listeners inside the sink is not preserved.
		template <R (*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7)>
		void RemoveListener(void)
		{
			Stub stub;
			stub.instance.as_void = nullptr;
			stub.function = &FunctionStub<Function>;

			listeners_.RemoveContiguous(stub);
		}

		/// \brief Removes a non-const class method as listener.
		/// \remark The order of listeners inside the sink is not preserved.
		template <class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7)>
		void RemoveListener(NonConstWrapper<C, Function> wrapper)
		{
			Stub stub;
			stub.instance.as_void = wrapper.instance_;
			stub.function = &ClassMethodStub<C, Function>;

			listeners_.RemoveContiguous(stub);
		}

		/// \brief Removes a const class method as listener.
		/// \remark The order of listeners inside the sink is not preserved.
		template <class C, R (C::*Function)(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7) const>
		void RemoveListener(ConstWrapper<C, Function> wrapper)
		{
			Stub stub;
			stub.instance.as_const_void = wrapper.instance_;
			stub.function = &ConstClassMethodStub<C, Function>;

			listeners_.RemoveContiguous(stub);
		}

		/// Returns the number of listeners.
		inline size_t GetListenerCount(void) const
		{
			return listeners_.Num();
		}

		/// Returns the i-th listener.
		inline const Stub& GetListener(size_t i) const
		{
			return listeners_[i];
		}

	private:
		X_NO_COPY(Sink);
		X_NO_ASSIGN(Sink);

		xArray<Stub> listeners_;
	};


	/// Default constructor.
	Event(void)
		: sink_(nullptr)
	{
	}

	/// Binds a sink to the event.
	inline void Bind(Sink* sink)
	{
		sink_ = sink;
	}

	/// Signals the sink.
	void Signal(ARG0 arg0, ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6, ARG7 arg7) const
	{
		X_ASSERT(sink_ != nullptr, "Cannot signal unbound event. Call Bind() first.")();

		for (size_t i=0; i<sink_->GetListenerCount(); ++i)
		{
			const Stub& stub = sink_->GetListener(i);
			stub.function(stub.instance , arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7);
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










X_NAMESPACE_END



