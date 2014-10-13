

template <typename T, size_t N>
inline FixedStack<T, N>::FixedStack(void)
: current_(array_)
{
}


template <typename T, size_t N>
void FixedStack<T, N>::push(const T& value)
{
	X_ASSERT(size() < capacity(), "can't push value on to full stack.")(size(), capacity());
	*current_++ = value;
}


template <typename T, size_t N>
void FixedStack<T, N>::pop(void)
{
	X_ASSERT(size() > 0, "can't pop value from a empty stack.")(size(), capacity());

	--current_;
}


template <typename T, size_t N>
inline T& FixedStack<T, N>::top(void)
{
	X_ASSERT(size() > 0, "can't get the topmost value of an empty stack.")(size(), capacity());
	return *(current_ - 1);
}


template <typename T, size_t N>
inline const T& FixedStack<T, N>::top(void) const
{
	X_ASSERT(size() > 0, "can't get the topmost value of an empty stack.")(size(), capacity());
	return *(current_ - 1);
}

template <typename T, size_t N>
inline bool FixedStack<T, N>::isEmpty(void) const
{
	return current_ == array_;
}

template <typename T, size_t N>
inline void FixedStack<T, N>::clear(void)
{
	current_ = array_; // move to start
}



template <typename T, size_t N>
inline size_t FixedStack<T, N>::size(void) const
{
	return safe_static_cast<size_t>(current_ - array_);
}

template <typename T, size_t N>
inline size_t FixedStack<T, N>::capacity(void) const
{
	return N;
}


template <typename T, size_t N>
inline typename FixedStack<T, N>::iterator FixedStack<T, N>::begin(void)
{
	return array_;
}

template <typename T, size_t N>
inline typename FixedStack<T, N>::const_iterator FixedStack<T, N>::begin(void) const
{
	return array_;
}

template <typename T, size_t N>
inline typename FixedStack<T, N>::iterator FixedStack<T, N>::end(void)
{
	return current_;
}

template <typename T, size_t N>
inline typename FixedStack<T, N>::const_iterator FixedStack<T, N>::end(void) const
{
	return current_;
}