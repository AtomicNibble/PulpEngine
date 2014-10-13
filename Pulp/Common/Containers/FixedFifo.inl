


template<typename T, size_t N>
FixedFifo<T, N>::FixedFifo() :
end_(array_ + N),
read_(array_),
write_(array_),
num_(0)
{

}

template<typename T, size_t N>
FixedFifo<T, N>::~FixedFifo()
{

}


template<typename T, size_t N>
void FixedFifo<T, N>::push(const T& v)
{
	if (num_ == capacity())
		Mem::Destruct<T>(write_);

	Mem::Construct<T>(write_, v);

	++write_;

	num_ = core::Min(++num_, capacity());

	if (write_ == end_)
		write_ = array_;
}

template<typename T, size_t N>
void FixedFifo<T, N>::pop(void)
{
	Mem::Destruct<T>(read_);

	++read_;
	--num_;

	if (read_ == end_)
		read_ = array_;
}

template<typename T, size_t N>
T& FixedFifo<T, N>::peek(void)
{
	return *read_;
}

template<typename T, size_t N>
const T& FixedFifo<T, N>::peek(void) const
{
	return *read_;
}



template<typename T, size_t N>
void FixedFifo<T, N>::clear(void)
{
	while (size() > 0)
		pop();

	num_ = 0;
	read_ = array_;
	write_ = array_;
}

template<typename T, size_t N>
typename FixedFifo<T, N>::size_type FixedFifo<T, N>::size(void) const
{
	return num_;
}

template<typename T, size_t N>
typename FixedFifo<T, N>::size_type FixedFifo<T, N>::capacity(void) const
{
	return end_ - array_;
}


// STL iterators.
template<typename T, size_t N>
typename FixedFifo<T, N>::iterator FixedFifo<T, N>::begin(void)
{
	return iterator(array_, end_, read_, 0);
}

template<typename T, size_t N>
typename FixedFifo<T, N>::iterator FixedFifo<T, N>::end(void)
{
	return iterator(array_, end_, write_, num_);
}

template<typename T, size_t N>
typename FixedFifo<T, N>::const_iterator FixedFifo<T, N>::begin(void) const
{
	return const_iterator(array_, end_, read_, 0);
}

template<typename T, size_t N>
typename FixedFifo<T, N>::const_iterator FixedFifo<T, N>::end(void) const
{
	return const_iterator(array_, end_, write_, num_);
}

/// ------------------------------------------------------

template<typename T, size_t N>
inline const T& FixedFifo<T, N>::iterator::operator*(void) const
{
	return *current_;
}

template<typename T, size_t N>
inline const T* FixedFifo<T, N>::iterator::operator->(void) const
{
	return current_;
}

template<typename T, size_t N>
inline typename FixedFifo<T, N>::iterator& FixedFifo<T, N>::iterator::operator++(void)
{
	++count_;
	++current_;
	if (current_ == end_)
		current_ = start_;

	return *this;
}

template<typename T, size_t N>
inline typename FixedFifo<T, N>::iterator FixedFifo<T, N>::iterator::operator++(int)
{
	iterator tmp = *this;
	++(*this); // call the function above.
	return tmp;
}

template<typename T, size_t N>
inline bool FixedFifo<T, N>::iterator::operator==(const iterator& rhs) const
{
	return count_ == rhs.count_;
}

template<typename T, size_t N>
inline bool FixedFifo<T, N>::iterator::operator!=(const iterator& rhs) const
{
	return count_ != rhs.count_;
}

/// ------------------------------------------------------

template<typename T, size_t N>
inline const T& FixedFifo<T, N>::const_iterator::operator*(void) const
{
	return *current_;
}

template<typename T, size_t N>
inline const T* FixedFifo<T, N>::const_iterator::operator->(void) const
{
	return current_;
}

template<typename T, size_t N>
inline typename FixedFifo<T, N>::const_iterator& FixedFifo<T, N>::const_iterator::operator++(void)
{
	++count_;
	++current_;
	if (current_ == end_)
		current_ = start_;

	return *this;
}

template<typename T, size_t N>
inline typename FixedFifo<T, N>::const_iterator FixedFifo<T, N>::const_iterator::operator++(int)
{
	const_iterator tmp = *this;
	++(*this); // call the function above.
	return tmp;
}

template<typename T, size_t N>
inline bool FixedFifo<T, N>::const_iterator::operator==(const const_iterator& rhs) const
{
	return count_ == rhs.count_;
}

template<typename T, size_t N>
inline bool FixedFifo<T, N>::const_iterator::operator!=(const const_iterator& rhs) const
{
	return count_ != rhs.count_;
}


