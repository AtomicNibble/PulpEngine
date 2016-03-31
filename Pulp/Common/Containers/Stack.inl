

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
Stack<T>::Stack(MemoryArenaBase* arena, size_type numElements) :
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
inline void Stack<T>::resize(size_type size)
{
	// same amount of items?
	if (newNum == num_)
		return;

	// grow?
	if (size > capacity()) 
	{
		Delete(start_);
		current_ = start_ = Allocate(size);
		end_ = (start_ + size);
	}
	else
	{
		// shrink.


	}
}

template<typename T>
void Stack<T>::resize(size_type size, const T&)
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
	size_type Size = size();
	for (size_t i = 0; i < Size; ++i) {
		Mem::Destruct<T>(start_ + i);
	}

	current_ = start_;
}

template<typename T>
inline typename Stack<T>::size_type Stack<T>::size() const
{
	return current_ - start_;
}

template<typename T>
inline typename Stack<T>::size_type Stack<T>::capacity() const
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

template<typename T>
inline typename Stack<T>::Iterator Stack<T>::begin(void)
{
	return start_;
}

template<typename T>
inline typename Stack<T>::ConstIterator Stack<T>::begin(void) const
{
	return start_;
}

template<typename T>
inline typename Stack<T>::Iterator Stack<T>::end(void)
{
	return current_;
}

template<typename T>
inline typename Stack<T>::ConstIterator Stack<T>::end(void) const
{
	return current_;
}


// allow for easy allocation custimisate later.
template<typename T>
inline void Stack<T>::Delete(T* pData)
{
	arena_->free(pData);
}

template<typename T>
inline T* Stack<T>::Allocate(size_type num)
{
	return static_cast<T*>(arena_->allocate(sizeof(T)*num,
		X_ALIGN_OF(T), 0, "Stack", "T[]", X_SOURCE_INFO));
}

