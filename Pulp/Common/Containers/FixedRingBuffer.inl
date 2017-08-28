
template<typename T, size_t N>
FixedRingBuffer<T,N>::FixedRingBuffer() :
	num_(0),
	head_(1),
	tail_(0)
{

}

template<typename T, size_t N>
FixedRingBuffer<T, N>::~FixedRingBuffer()
{
	clear();
}

template<typename T, size_t N>
void FixedRingBuffer<T, N>::clear(void)
{
	size_type i;

	for (i = 0; i < num_; i++) {
		Mem::Destruct(&array_ + index_to_subscript(i));
	}

	num_ = 0;
}

template<typename T, size_t N>
typename FixedRingBuffer<T, N>::size_type FixedRingBuffer<T, N>::size(void) const
{
	return num_;
}

template<typename T, size_t N>
typename FixedRingBuffer<T, N>::size_type FixedRingBuffer<T, N>::capacity(void) const
{
	return N;
}

template<typename T, size_t N>
void FixedRingBuffer<T, N>::append(const T& val)
{
	size_type next = next_tail();
	if (num_ == N)
	{
		array_[next] = val;
		increment_head();
	}
	else
	{
		Mem::Construct(array_ + next, val);
	}

	increment_tail();
}

template<typename T, size_t N>
void FixedRingBuffer<T, N>::push_back(const T& val)
{
	append(val);
}


// -----------------------------------

template<typename T, size_t N>
typename FixedRingBuffer<T, N>::iterator FixedRingBuffer<T, N>::begin()
{ 
	return iterator(this, 0);
}

template<typename T, size_t N>
typename FixedRingBuffer<T, N>::iterator FixedRingBuffer<T, N>::end()
{ 
	return iterator(this, size()); 
}

template<typename T, size_t N>
typename FixedRingBuffer<T, N>::const_iterator FixedRingBuffer<T, N>::begin() const
{
	return const_iterator(this, 0); 
}

template<typename T, size_t N>
typename FixedRingBuffer<T, N>::const_iterator FixedRingBuffer<T, N>::end() const
{ 
	return const_iterator(this, size()); 
}

// -----------------------------------

template<typename T, size_t N>
typename FixedRingBuffer<T, N>::reverse_iterator FixedRingBuffer<T, N>::rbegin()
{
	return reverse_iterator(end());
}

template<typename T, size_t N>
typename FixedRingBuffer<T, N>::reverse_iterator FixedRingBuffer<T, N>::rend()
{
	return reverse_iterator(begin());
}

template<typename T, size_t N>
typename FixedRingBuffer<T, N>::const_reverse_iterator FixedRingBuffer<T, N>::rbegin() const
{
	return const_reverse_iterator(end());
}

template<typename T, size_t N>
typename FixedRingBuffer<T, N>::const_reverse_iterator FixedRingBuffer<T, N>::rend() const
{
	return const_reverse_iterator(begin());
}


// -----------------------------------

template<typename T, size_t N>
typename FixedRingBuffer<T, N>::reference FixedRingBuffer<T, N>::operator[](size_type idx)
{ 
	return array_[index_to_subscript(idx)];
}

template<typename T, size_t N>
typename FixedRingBuffer<T, N>::const_reference FixedRingBuffer<T, N>::operator[](size_type idx) const
{ 
	return array_[index_to_subscript(idx)];
}


// -----------------------------------
template<typename T, size_t N>
typename FixedRingBuffer<T, N>::size_type FixedRingBuffer<T, N>::normalise(size_type idx) const
{
	return idx % N;
}

template<typename T, size_t N>
typename FixedRingBuffer<T, N>::size_type FixedRingBuffer<T, N>::index_to_subscript(size_type idx) const
{
	return normalise(idx + head_);
}

template<typename T, size_t N>
typename FixedRingBuffer<T, N>::size_type FixedRingBuffer<T, N>::next_tail()
{
	return (tail_ + 1 == N) ? 0 : tail_ + 1;
}

template<typename T, size_t N>
void FixedRingBuffer<T, N>::increment_tail()
{
	++num_;
	tail_ = next_tail();
}

template<typename T, size_t N>
void FixedRingBuffer<T, N>::increment_head()
{
	++head_;
	--num_;
	if (head_ == N)
		head_ = 0;
}
