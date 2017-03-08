


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
Fifo<T>::Fifo(const Fifo& oth) : Fifo<T>(oth.arena_, oth.capacity())
{
	// support nonePod
	Mem::CopyArrayUninitialized(start_, oth.start_, oth.start_ + oth.size());

	// set read/write/num
	read_ = start_ + (oth.read_ - oth.start_);
	write_ = start_ + (oth.write_ - oth.start_);
	num_ = oth.num_;
}

template<typename T>
Fifo<T>::Fifo(Fifo&& oth)
{
	start_ = oth.start_;
	end_ = oth.end_;
	read_ = oth.read_;
	write_ = oth.write_;

	num_ = oth.num_;

	arena_ = oth.arena_;
	
	// clear oth.
	oth.start_ = nullptr;
	oth.end_ = nullptr;
	oth.read_ = nullptr;
	oth.write_ = nullptr;
	oth.num_ = 0;
}

template<typename T>
Fifo<T>::~Fifo()
{
	free();
}

template<typename T>
Fifo<T>& Fifo<T>::operator = (const Fifo<T>& oth)
{
	if (this != &oth)
	{
		free();

		arena_ = oth.arena_;

		reserve(oth.capacity());

		Mem::CopyArrayUninitialized(start_, oth.start_, oth.start_ + oth.size());

		// set read/write/num
		read_ = start_ + (oth.read_ - oth.start_);
		write_ = start_ + (oth.write_ - oth.start_);
		num_ = oth.num_;
	}
	return *this;
}

template<typename T>
Fifo<T>& Fifo<T>::operator = (Fifo<T>&& oth)
{
	if (this != &oth)
	{
		free();

		start_ = oth.start_;
		end_ = oth.end_;
		read_ = oth.read_;
		write_ = oth.write_;

		num_ = oth.num_;

		arena_ = oth.arena_;

		// clear oth.
		oth.start_ = nullptr;
		oth.end_ = nullptr;
		oth.read_ = nullptr;
		oth.write_ = nullptr;
		oth.num_ = 0;
	}
	return *this;
}


template<typename T>
void Fifo<T>::setArena(MemoryArenaBase* arena)
{
	X_ASSERT(arena_ == nullptr || num_ == 0, "can't set arena on a Fifo that has items")(num_);
	arena_ = arena;
}

template<typename T>
X_INLINE T& Fifo<T>::operator[](size_type idx)
{
	X_ASSERT(idx < size(), "Index out of range.")(idx, size());

	if (read_ + idx > end_) {
		size_type left = end_ - read_;
		return *(start_ + (idx - left));
	}

	return *(read_ + idx);
}

template<typename T>
X_INLINE const T& Fifo<T>::operator[](size_type idx) const
{
	X_ASSERT(idx < size(), "Index out of range.")(idx, size());

	if (read_ + idx > end_) {
		size_type left = end_ - read_;
		return *(start_ + (idx - left));
	}

	return *(read_ + idx);
}

template<typename T>
void Fifo<T>::push(const T& v)
{
	X_ASSERT(size() < capacity(), "Cannot push another value into an already full FIFO.")(size(), capacity());


	Mem::Construct<T>(write_, v);

	++write_;

	num_ = core::Min(++num_, capacity());

	if (write_ == end_) {
		write_ = start_;
	}
}


template<typename T>
void Fifo<T>::push(T&& v)
{
	X_ASSERT(size() < capacity(), "Cannot push another value into an already full FIFO.")(size(), capacity());

	Mem::Construct<T>(write_, std::forward<T>(v));

	++write_;

	num_ = core::Min(++num_, capacity());

	if (write_ == end_) {
		write_ = start_;
	}
}

template<typename T>
template<class... ArgsT>
void Fifo<T>::emplace(ArgsT&&... args)
{
	X_ASSERT(size() < capacity(), "Cannot emplace another value into an already full FIFO.")(size(), capacity());

	Mem::Construct<T>(write_, std::forward<ArgsT>(args)...);

	++write_;

	num_ = core::Min(++num_, capacity());

	if (write_ == end_) {
		write_ = start_;
	}
}


template<typename T>
void Fifo<T>::pop(void)
{
	X_ASSERT(!isEmpty(), "Cannot pop value of an empty FIFO.")(size(), capacity());

	Mem::Destruct<T>(read_);

	++read_;
	--num_;

	if (read_ == end_) {
		read_ = start_;
	}
}

template<typename T>
T& Fifo<T>::peek(void)
{
	X_ASSERT(!isEmpty(), "Cannot access the frontmost value of an empty FIFO.")(size(), capacity());
	return *read_;
}

template<typename T>
const T& Fifo<T>::peek(void) const
{
	X_ASSERT(!isEmpty(), "Cannot access the frontmost value of an empty FIFO.")(size(), capacity());
	return *read_;
}


template<typename T>
bool Fifo<T>::contains(const T& oth) const
{
	auto endIt = end();
	for (auto it = begin(); it != endIt; ++it)
	{
		if (*it == oth) {
			return true;
		}
	}

	return false;
}

template<typename T>
template<class UnaryPredicate>
bool Fifo<T>::contains_if(UnaryPredicate p) const
{
	auto endIt = end();
	for (auto it = begin(); it != endIt; ++it)
	{
		if (p(*it)) {
			return true;
		}
	}

	return false;
}


template<typename T>
void Fifo<T>::reserve(size_type num)
{
	X_ASSERT(start_ == nullptr, "Cannot reserve additional memory. free() the FIFO first.")(
		size(), capacity(), start_, end_, num);


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

template<typename T>
bool Fifo<T>::isNotEmpty(void) const
{
	return num_ != 0;
}
// ----------------------------------------------

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

/// ------------------------------------------------------c

template<typename T>
typename Fifo<T>::Reference Fifo<T>::front(void)
{
	return *read_;
}

template<typename T>
typename Fifo<T>::ConstReference Fifo<T>::front(void) const
{
	return *read_;
}

template<typename T>
typename Fifo<T>::Reference Fifo<T>::back(void)
{
	return *(write_ - 1);

}

template<typename T>
typename Fifo<T>::ConstReference Fifo<T>::back(void) const
{
	return *(write_ - 1);
}

/// ------------------------------------------------------c



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
	// don't deconstruct
	uint8_t* pDataPod = union_cast<uint8_t*, T*>(pData);

	X_DELETE_ARRAY(pDataPod, arena_);
}

template<typename T>
T* Fifo<T>::Allocate(size_type num)
{
	return reinterpret_cast<T*>(X_NEW_ARRAY(uint8_t, num * sizeof(T), arena_, "Fifo<"X_PP_STRINGIZE(T)">"));
}