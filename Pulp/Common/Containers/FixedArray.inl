

template<typename T, size_t N>
FixedArray<T, N>::FixedArray() :
	size_(0)
{
}


template<typename T, size_t N>
FixedArray<T, N>::FixedArray(const T& initial) :
	size_(0)
{
	T* pArr = begin();

	for (size_type i = 0; i < N; ++i) {
		Mem::Construct(&pArr[i], initial);
	}

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
	const T* pArr = begin();

	return pArr[idx];
}

template<typename T, size_t N>
T& FixedArray<T, N>::operator[](size_type idx)
{
	X_ASSERT(idx >= 0 && idx < N, "Array index out of bounds")(idx, N);
	T* pArr = begin();

	return pArr[idx];
}

// returns a pointer to the list
template<typename T, size_t N>
T* FixedArray<T, N>::ptr(void)
{
	return begin();
}

template<typename T, size_t N>
const T* FixedArray<T, N>::ptr(void) const
{
	return begin();
}


// clear the list, no memory free
template<typename T, size_t N>
inline void FixedArray<T, N>::clear(void)
{
	size_t i;
	T* pArr = begin();

	for (i = 0; i<size_; ++i)
		Mem::Destruct(pArr + i);

	size_ = 0;
}

// append blank elemtnt and return refrence
template<typename T, size_t N>
template<class... Args>
X_INLINE typename FixedArray<T, N>::Type& FixedArray<T, N>::AddOne(Args&&... args)
{
	X_ASSERT(size_ < N, "Fixed size array is full")(N, size_);
	T* pArr = begin();

	Mem::Construct<T>(&pArr[size_], std::forward<Args>(args)...);

	return pArr[size_++];
}

// append element (same as push_back)
template<typename T, size_t N>
typename FixedArray<T, N>::size_type FixedArray<T, N>::append(const T& obj)
{
	X_ASSERT(size_ < N, "Fixed size array is full")(N, size_);
	T* pArr = begin();

	Mem::Construct(&pArr[size_], obj);

	size_++;
	return size_ - 1;
}

template<typename T, size_t N>
typename FixedArray<T, N>::size_type FixedArray<T, N>::append(T&& obj)
{
	X_ASSERT(size_ < N, "Fixed size array is full")(N, size_);
	T* pArr = begin();

	Mem::Construct(&pArr[size_], std::forward<T>(obj));

	size_++;
	return size_ - 1;
}

// appends a item to the end, resizing if required.
template<typename T, size_t N>
typename FixedArray<T, N>::size_type FixedArray<T, N>::push_back(const T& obj)
{
	X_ASSERT(size_ < N, "Fixed size array is full")(N, size_);
	T* pArr = begin();

	Mem::Construct(&pArr[size_], obj);

	size_++;
	return size_ - 1;
}

template<typename T, size_t N>
typename FixedArray<T, N>::size_type FixedArray<T, N>::push_back(T&& obj)
{
	X_ASSERT(size_ < N, "Fixed size array is full")(N, size_);
	T* pArr = begin();

	Mem::Construct(&pArr[size_], std::forward<T>(obj));

	size_++;
	return size_ - 1;
}

template<typename T, size_t N>
template<class... ArgsT>
typename FixedArray<T, N>::size_type FixedArray<T, N>::emplace_back(ArgsT&&... args)
{
	X_ASSERT(size_ < N, "Fixed size array is full")(N, size_);
	T* pArr = begin();

	Mem::Construct<T>(&pArr[size_], std::forward<ArgsT>(args)...);

	size_++;
	return size_ - 1;
}

template<typename T, size_t N>
typename FixedArray<T, N>::iterator FixedArray<T, N>::insert(iterator position, const T& val)
{
	size_type off = position - begin();

	X_ASSERT(off < N, "invalid position for FixedArray: out of bounds")(N, off);

	T* pArr = begin();

	if (off < size_)
	{
		// move them down.
		for (size_type i = size_; i > off; --i)
			pArr[i] = pArr[i - 1];
	}

	pArr[off] = val;
	size_++;
	return position;
}

template<typename T, size_t N>
bool FixedArray<T, N>::removeIndex(size_type idx)
{
	if (idx < size() && isNotEmpty())
	{
		T* pArr = begin();

		size_--; // remove first so we don't try access it.

		// shift them down.
		size_type i, num;
		for (i = idx, num = size_; i < num; i++)
			pArr[i] = pArr[i + 1];

		Mem::Destruct(pArr + size_);
		return true;
	}
	return false;
}

template<typename T, size_t N>
bool FixedArray<T, N>::remove(iterator position)
{
	size_type idx = position - begin();
	return removeIndex(idx);
}


template<typename T, size_t N>
void FixedArray<T, N>::resize(size_type newNum, const T& t)
{
	X_ASSERT(newNum >= 0, "array size must be positive")(newNum);
	X_ASSERT(newNum < N, "array size must be less or equal to capacity")(newNum, N);
	
	if (newNum == size_) {
		return;
	}

	T* pArr = begin();

	// remove some?
	if (newNum < size_)
	{
		// we don't delete memory just deconstruct.
		Mem::DestructArray<T>(&pArr[newNum], size_ - newNum);
	}
	else
	{
		size_type	i;

		// construct the new items.
		for (i = size_; i < newNum; i++) {
			Mem::Construct<T>(&pArr[i], t);
		}
	}

	// set num
	size_ = newNum;
}

// any iterms in the array
template<typename T, size_t N>
bool FixedArray<T, N>::isEmpty(void) const
{
	return size_ == 0;
}

template<typename T, size_t N>
bool FixedArray<T, N>::isNotEmpty(void) const
{
	return size_ > 0;
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
	return reinterpret_cast<FixedArray<T, N>::iterator>(array_);
}

template<typename T, size_t N>
inline typename FixedArray<T, N>::const_iterator FixedArray<T, N>::begin(void) const
{
	return reinterpret_cast<FixedArray<T, N>::const_iterator>(array_);
}

template<typename T, size_t N>
inline typename FixedArray<T, N>::iterator FixedArray<T, N>::end(void)
{
	return begin() + size_;
}

template<typename T, size_t N>
inline typename FixedArray<T, N>::const_iterator FixedArray<T, N>::end(void) const
{
	return begin() + size_;
}


template<typename T, size_t N>
inline typename FixedArray<T, N>::Reference FixedArray<T, N>::front(void)
{
	X_ASSERT(isNotEmpty(), "Array can't be empty when calling front")(isNotEmpty());
	return *begin();
}

template<typename T, size_t N>
inline typename FixedArray<T, N>::ConstReference FixedArray<T, N>::front(void) const
{
	X_ASSERT(isNotEmpty(), "Array can't be empty when calling front")(isNotEmpty());
	return *begin();
}


template<typename T, size_t N>
inline typename FixedArray<T, N>::Reference FixedArray<T, N>::back(void)
{
	X_ASSERT(isNotEmpty(), "Array can't be empty when calling back")(isNotEmpty());
	return (*(end() - 1));
}

template<typename T, size_t N>
inline typename FixedArray<T, N>::ConstReference FixedArray<T, N>::back(void) const
{
	X_ASSERT(isNotEmpty(), "Array can't be empty when calling back")(isNotEmpty());
	return (*(end() - 1));
}
