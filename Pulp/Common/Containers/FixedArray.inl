

template<typename T, size_t N>
FixedArray<T, N>::FixedArray() :
	size_(0)
{
//	for (size_type i = 0; i<N; ++i)
//		array_[i] = T();
}


template<typename T, size_t N>
FixedArray<T, N>::FixedArray(const T& initial) :
	size_(0)
{
	for (size_type i = 0; i < N; ++i)
		array_[i] = initial;
	size_ = N;
}


template<typename T, size_t N>
FixedArray<T, N>::~FixedArray(void)
{
	clear();
}


// index operators
template<typename T, size_t N>
const T& FixedArray<T, N>::operator[](size_type idx) const
{
	X_ASSERT(idx >= 0 && idx < N, "Array index out of bounds")(idx, N);
	return array_[idx];
}

template<typename T, size_t N>
T& FixedArray<T, N>::operator[](size_type idx)
{
	X_ASSERT(idx >= 0 && idx < N, "Array index out of bounds")(idx, N);
	return array_[idx];
}

// returns a pointer to the list
template<typename T, size_t N>
T* FixedArray<T, N>::ptr(void)
{
	return array_;
}

template<typename T, size_t N>
const T* FixedArray<T, N>::ptr(void) const
{
	return array_;
}


// clear the list, no memory free
template<typename T, size_t N>
inline void FixedArray<T, N>::clear(void)
{
	size_t i;

	for (i = 0; i<size_; ++i)
		Mem::Destruct(array_ + i);

	size_ = 0;
}

// append element (same as push_back)
template<typename T, size_t N>
typename FixedArray<T, N>::size_type FixedArray<T, N>::append(const T& obj)
{
	X_ASSERT(size_ < N, "Fixed size stack is full")(N, size_);

	Mem::Construct(&array_[size_], obj);

	size_++;
	return size_ - 1;
}

// appends a item to the end, resizing if required.
template<typename T, size_t N>
typename FixedArray<T, N>::size_type FixedArray<T, N>::push_back(const T& obj)
{
	X_ASSERT(size_ < N, "Fixed size array is full")(N, size_);

	Mem::Construct(&array_[size_], obj);

	size_++;
	return size_ - 1;
}

template<typename T, size_t N>
typename FixedArray<T, N>::iterator FixedArray<T, N>::insert(iterator position, const T& val)
{
	size_type off = position - begin();

	X_ASSERT(off < N, "invalid position for FixedArray: out of bounds")(N, off);


	if (off < size_)
	{
		// move them down.
		for (size_type i = size_; i > off; --i)
			array_[i] = array_[i - 1];
	}

	array_[off] = val;
	size_++;
	return position;
}


// any iterms in the array
template<typename T, size_t N>
bool FixedArray<T, N>::isEmpty(void) const
{
	return size_ == 0;
}


template<typename T, size_t N>
typename FixedArray<T, N>::size_type FixedArray<T, N>::size(void) const
{
	return size_;
}

// returns number of elements allocated for
template<typename T, size_t N>
typename FixedArray<T, N>::size_type FixedArray<T, N>::capacity(void) const
{
	return N;
}


// -----------------------------------------------

template<typename T, size_t N>
inline typename FixedArray<T, N>::iterator FixedArray<T, N>::begin(void)
{
	return array_;
}

template<typename T, size_t N>
inline typename FixedArray<T, N>::const_iterator FixedArray<T, N>::begin(void) const
{
	return array_;
}

template<typename T, size_t N>
inline typename FixedArray<T, N>::iterator FixedArray<T, N>::end(void)
{
	return array_ + size_;
}

template<typename T, size_t N>
inline typename FixedArray<T, N>::const_iterator FixedArray<T, N>::end(void) const
{
	return array_ + size_;
}

