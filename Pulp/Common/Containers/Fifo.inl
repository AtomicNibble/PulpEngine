


template<typename T>
Fifo<T>::Fifo(MemoryArenaBase* arena) :
	start_(nullptr),
	end_(nullptr),
	read_(nullptr),
	write_(nullptr),
	num_(0),
	arena_(arena)
{

}

template<typename T>
Fifo<T>::Fifo(MemoryArenaBase* arena, size_type size) :
	start_(nullptr),
	end_(nullptr),
	read_(nullptr),
	write_(nullptr),
	num_(0),
	arena_(arena)
{
	X_ASSERT_NOT_NULL(arena);
	reserve(size);
}

template<typename T>
Fifo<T>::~Fifo()
{
	free();
}


template<typename T>
void Fifo<T>::setArena(MemoryArenaBase* arena)
{
	X_ASSERT(arena_ == nullptr || num_ == 0, "can't set arena on a Fifo that has items")(num_);
	arena_ = arena;
}

template<typename T>
void Fifo<T>::push(const T& v)
{
	if (num_ == capacity())
		Mem::Destruct<T>(write_);

	Mem::Construct<T>(write_, v);

	++write_;

	num_ = core::Min(++num_, capacity());

	if (write_ == end_)
		write_ = start_;
}

template<typename T>
void Fifo<T>::pop(void)
{
	Mem::Destruct<T>(read_);

	++read_;
	--num_;

	if (read_ == end_)
		read_ = start_;
}

template<typename T>
T& Fifo<T>::peek(void)
{
	return *read_;
}

template<typename T>
const T& Fifo<T>::peek(void) const
{
	return *read_;
}

template<typename T>
void Fifo<T>::reserve(size_type num)
{
	start_ = Allocate(num);
	end_ = start_ + num;
	read_ = start_;
	write_ = start_;
}

template<typename T>
void Fifo<T>::clear(void)
{
	while (size() > 0)
		pop();

	num_ = 0;
	read_ = start_;
	write_ = start_;
}


template<typename T>
void Fifo<T>::free(void)
{
	clear();
	Delete(start_);

	start_ = nullptr;
	end_ = nullptr;
}


template<typename T>
typename Fifo<T>::size_type Fifo<T>::size(void) const
{
	return num_;
}

template<typename T>
typename Fifo<T>::size_type Fifo<T>::capacity(void) const
{
	return end_ - start_;
}

template<typename T>
bool Fifo<T>::isEmpty(void) const
{
	return num_ == 0;
}

// STL iterators.
template<typename T>
typename Fifo<T>::iterator Fifo<T>::begin(void)
{
	return iterator(start_, end_, read_, 0);
}

template<typename T>
typename Fifo<T>::iterator Fifo<T>::end(void)
{
	return iterator(start_, end_, write_, num_);
}

template<typename T>
typename Fifo<T>::const_iterator Fifo<T>::begin(void) const
{
	return const_iterator(start_, end_, read_, 0);
}

template<typename T>
typename Fifo<T>::const_iterator Fifo<T>::end(void) const
{
	return const_iterator(start_, end_, write_, num_);
}

/// ------------------------------------------------------

template<typename T>
inline const T& Fifo<T>::iterator::operator*(void) const
{
	return *current_;
}

template<typename T>
inline const T* Fifo<T>::iterator::operator->(void) const
{
	return current_;
}

template<typename T>
inline typename Fifo<T>::iterator& Fifo<T>::iterator::operator++(void)
{
	++count_;
	++current_;
	if (current_ == end_)
		current_ = start_;

	return *this;
}

template<typename T>
inline typename Fifo<T>::iterator Fifo<T>::iterator::operator++(int)
{
	iterator tmp = *this;
	++(*this); // call the function above.
	return tmp;
}

template<typename T>
inline bool Fifo<T>::iterator::operator==(const iterator& rhs) const
{
	return count_ == rhs.count_;
}

template<typename T>
inline bool Fifo<T>::iterator::operator!=(const iterator& rhs) const
{
	return count_ != rhs.count_;
}

/*
template<typename T>
inline Fifo<T>::iterator::operator typename Fifo<T>::iterator::const_iterator(void) const
{
	return const_iterator(start_, end_, current_, count_);
}*/

/// ------------------------------------------------------

template<typename T>
inline const T& Fifo<T>::const_iterator::operator*(void) const
{
	return *current_;
}

template<typename T>
inline const T* Fifo<T>::const_iterator::operator->(void) const
{
	return current_;
}

template<typename T>
inline typename Fifo<T>::const_iterator& Fifo<T>::const_iterator::operator++(void)
{
	++count_;
	++current_;
	if (current_ == end_)
		current_ = start_;

	return *this;
}

template<typename T>
inline typename Fifo<T>::const_iterator Fifo<T>::const_iterator::operator++(int)
{
	const_iterator tmp = *this;
	++(*this); // call the function above.
	return tmp;
}

template<typename T>
inline bool Fifo<T>::const_iterator::operator==(const const_iterator& rhs) const
{
	return count_ == rhs.count_;
}

template<typename T>
inline bool Fifo<T>::const_iterator::operator!=(const const_iterator& rhs) const
{
	return count_ != rhs.count_;
}




/// ----------------------------

template<typename T>
void Fifo<T>::Delete(T* pData)
{
	X_DELETE_ARRAY(pData, arena_);
//	delete[] pData;
}

template<typename T>
T* Fifo<T>::Allocate(size_type num)
{
	return X_NEW_ARRAY(T, num, arena_, "Fifo<"X_PP_STRINGIZE(T)">");
//	return new T[num];
}