

// constructs the Stack no memory is allocated.
template<typename T>
Stack<T>::Stack(MemoryArenaBase* arena) :
current_(nullptr),
end_(nullptr),
start_(nullptr),
arena_(arena)
{

}

template<typename T>
Stack<T>::Stack(MemoryArenaBase* arena, size_t numElements) :
current_(nullptr),
end_(nullptr),
start_(nullptr),
arena_(arena)
{
	X_ASSERT_NOT_NULL(arena);
	resize(numElements);
}


template<typename T>
inline void Stack<T>::SetArena(MemoryArenaBase* arena)
{
	X_ASSERT(arena_ == nullptr || (capacity() == 0 && size() == 0), "can't set arena on a Fifo that has items")(size(), capacity());
	arena_ = arena;
}

// push a value onto the stack
template<typename T>
inline void Stack<T>::push(const T& val)
{
	X_ASSERT(size() < capacity(), "can't push value onto stack, no room.")(size(), capacity());

	Mem::Construct<T>(current_, val);
	++current_;
}

// emplace a value onto the stack
template<typename T>
template<class... ArgsT>
inline void Stack<T>::emplace(ArgsT&&... args)
{
	X_ASSERT(size() < capacity(), "can't push value onto stack, no room.")(size(), capacity());

	Mem::Construct<T>(current_, std::forward<ArgsT>(args)...);
	++current_;
}


// pop a value of the stack
template<typename T>
inline void Stack<T>::pop(void)
{
	X_ASSERT(size() > 0, "can't pop from a empty stack")(size(), capacity());
	
	--current_;
	Mem::Destruct<T>(current_);
}

// get the top item without removing
template<typename T>
inline T& Stack<T>::top(void)
{
	X_ASSERT(size() > 0, "can't get top from a empty stack")(size(), capacity());
	return *(current_ - 1);
}

// get the top item without removing
template<typename T>
inline const T& Stack<T>::top(void) const
{
	X_ASSERT(size() > 0, "can't get top from a empty stack")(size(), capacity());
	return *(current_ - 1);
}




// resizes the object
template<typename T>
inline void Stack<T>::resize(size_t size)
{
	if (size > capacity()) // we need to resize?
	{
		Delete(start_);
		current_ = start_ = Allocate(size);
		end_ = (start_ + size);
	}
}


// free's the memory associated with the stack.
template<typename T>
inline void Stack<T>::free(void)
{
	clear();

	if (start_) {
		Delete(start_);
	}
	current_ = nullptr;
	start_ = nullptr;
	end_ = nullptr;
}

// clears all objects but dose not free memory.
template<typename T>
inline void Stack<T>::clear(void)
{
	size_t Size = size();
	for (size_t i = 0; i<Size; ++i)
		Mem::Destruct<T>(start_ + i);

	current_ = start_;
}

template<typename T>
inline size_t Stack<T>::size() const
{
	return current_ - start_;
}

template<typename T>
inline size_t Stack<T>::capacity() const
{
	return end_ - start_;
}

template<typename T>
inline bool Stack<T>::isEmpty(void) const
{
	return size() == 0;
}

template<typename T>
inline bool Stack<T>::isNotEmpty(void) const
{
	return size() > 0;
}

// allow for easy allocation custimisate later.
template<typename T>
inline void Stack<T>::Delete(T* pData)
{
	X_DELETE_ARRAY(pData, arena_);
	//::free(pData);
}

template<typename T>
inline T* Stack<T>::Allocate(size_t num)
{
	return X_NEW_ARRAY(T, num, arena_, "Stack<"X_PP_STRINGIZE(T)">");
//	return (T*)malloc(sizeof(T)*num);
}