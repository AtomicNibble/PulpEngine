
template<typename T>
X_INLINE AlignedArray<T>::AlignedArray(MemoryArenaBase* arena) :
	granularity_(16),
	list_(nullptr),
	num_(0),
	size_(0),
	align_(X_ALIGN_OF(T)),
	arena_(arena)
{
	//	X_ASSERT_NOT_NULL(arena);
}


template<typename T>
X_INLINE AlignedArray<T>::AlignedArray(MemoryArenaBase* arena, size_type size) :
	granularity_(16),
	list_(nullptr),
	num_(size),
	size_(size),
	align_(X_ALIGN_OF(T)),
	arena_(arena)
{
	X_ASSERT(size > 0, "List size must be positive")(size);
	X_ASSERT_NOT_NULL(arena);

	list_ = Allocate(size);

	for (size_type i = 0; i<size; ++i)
		Mem::Construct<T>(list_ + i);
}

template<typename T>
X_INLINE AlignedArray<T>::AlignedArray(MemoryArenaBase* arena, size_type size, const T& initialValue) :
	granularity_(16),
	list_(nullptr),
	num_(size),
	size_(size),
	align_(X_ALIGN_OF(T)),
	arena_(arena)
{
	X_ASSERT(size > 0, "List size must be positive")(size);
	X_ASSERT_NOT_NULL(arena);

	list_ = Allocate(size);

	for (size_type i = 0; i < size; ++i) {
		Mem::Construct<T>(list_ + i, initialValue);
	}
}

template<typename T>
X_INLINE AlignedArray<T>::AlignedArray(MemoryArenaBase* arena, std::initializer_list<T> iList) :
	AlignedArray(arena)
{
	align_ = X_ALIGN_OF(T);
	size_t size = iList.size();
	std::initializer_list<T>::const_iterator pList = iList.begin();

	ensureSize(size);

	for (size_type i = 0; i < size; ++i) {
		Mem::Construct<T>(list_ + i, pList[i]);
	}

	num_ = size;
}

template<typename T>
X_INLINE AlignedArray<T>::AlignedArray(const AlignedArray<T>& oth) :
	granularity_(16),
	list_(nullptr),
	num_(0),
	size_(0),
	align_(X_ALIGN_OF(T))
{
	*this = oth;
}


template<typename T>
X_INLINE AlignedArray<T>::AlignedArray(AlignedArray<T>&& oth) :
	granularity_(16),
	list_(nullptr),
	num_(0),
	size_(0),
	align_(X_ALIGN_OF(T))
{
	list_ = oth.list_;
	num_ = oth.num_;
	size_ = oth.size_;
	granularity_ = oth.granularity_;
	arena_ = oth.arena_;

	// clear other.
	oth.list_ = nullptr;
	oth.num_ = 0;
	oth.size_ = 0;
}


template<typename T>
X_INLINE AlignedArray<T>::~AlignedArray(void)
{
	free();
}

template<typename T>
X_INLINE void AlignedArray<T>::setArena(MemoryArenaBase* arena)
{
	X_ASSERT(arena_ == nullptr || num_ == 0, "can't set arena on a array that has items")(num_);
	arena_ = arena;
}

template<typename T>
X_INLINE void AlignedArray<T>::setArena(MemoryArenaBase* arena, size_type capacity)
{
	X_ASSERT(arena_ == nullptr || num_ == 0, "can't set arena on a array that has items")(num_);
	arena_ = arena;

	reserve(capacity);
}

// ---------------------------------------------------------

template<typename T>
AlignedArray<T>& AlignedArray<T>::operator=(std::initializer_list<T> iList)
{
	size_type i;
	free();

	size_t size = iList.size();
	std::initializer_list<T>::const_iterator pList = iList.begin();

	if (size) {
		ensureSize(size);
		for (i = 0; i < size; i++) {
			Mem::Construct(&list_[i], pList[i]);
		}
	}

	num_ = size;
	return *this;
}

template<typename T>
AlignedArray<T>& AlignedArray<T>::operator=(const AlignedArray<T> &oth)
{
	size_type i;
	free();

	num_ = oth.num_;
	size_ = oth.size_;
	granularity_ = oth.granularity_;
	arena_ = oth.arena_;

	if (size_) {
		list_ = Allocate(size_);
		for (i = 0; i < num_; i++) {
			Mem::Construct(&list_[i], oth.list_[i]);
		}
	}

	return *this;
}

template<typename T>
AlignedArray<T>& AlignedArray<T>::operator=(AlignedArray<T>&& oth)
{
	if (this != &oth)
	{
		free();

		list_ = oth.list_;
		num_ = oth.num_;
		size_ = oth.size_;
		granularity_ = oth.granularity_;
		arena_ = oth.arena_;

		// clear other.
		oth.list_ = nullptr;
		oth.num_ = 0;
		oth.size_ = 0;
	}
	return *this;
}

template<typename T>
X_INLINE const T&AlignedArray<T>::operator[](size_type idx) const {
	X_ASSERT(idx >= 0 && idx < num_, "Array index out of bounds")(idx, num_);
	return list_[idx];
}


template<typename T>
X_INLINE T&AlignedArray<T>::operator[](size_type idx) {
	X_ASSERT(idx >= 0 && idx < num_, "Array index out of bounds")(idx, num_);
	return list_[idx];
}

// ---------------------------------------------------------

template<typename T>
X_INLINE T *AlignedArray<T>::ptr(void) {
	return list_;
}


template<typename T>
const X_INLINE T *AlignedArray<T>::ptr(void) const {
	return list_;
}

template<typename T>
X_INLINE T *AlignedArray<T>::data(void) {
	if (isNotEmpty()) {
		return &front();
	}

	return nullptr;
}


template<typename T>
const X_INLINE T *AlignedArray<T>::data(void) const {
	if (isNotEmpty()) {
		return &front();
	}

	return nullptr;
}

// ---------------------------------------------------------

template<typename T>
X_INLINE const bool AlignedArray<T>::isEmpty(void) const
{
	return num_ == 0;
}

template<typename T>
X_INLINE const bool AlignedArray<T>::isNotEmpty(void) const
{
	return num_ > 0;
}

template<typename T>
X_INLINE void AlignedArray<T>::clear(void)
{
	// properly destruct the instances
	size_t i;

	for (i = 0; i < size(); ++i) {
		Mem::Destruct(list_ + i);
	}

	num_ = 0; // don't free any memory
}

template<typename T>
X_INLINE void AlignedArray<T>::free(void)
{
	clear(); // make sure to destruct the objects.

	if (list_) {
		DeleteArr(list_);
	}

	list_ = nullptr;
	num_ = 0;
	size_ = 0;
}


template<typename T>
X_INLINE typename AlignedArray<T>::size_type AlignedArray<T>::size(void) const {
	return num_;
}


template<typename T>
X_INLINE typename AlignedArray<T>::size_type AlignedArray<T>::capacity(void) const {
	return size_;
}


template<typename T>
X_INLINE void AlignedArray<T>::setGranularity(size_type newgranularity)
{
	X_ASSERT(newgranularity >= 0, "granularity size must be positive")(newgranularity);

	granularity_ = newgranularity;
}


template<typename T>
X_INLINE typename AlignedArray<T>::size_type AlignedArray<T>::granularity(void) const {
	return granularity_;
}



template <typename T>
X_INLINE void AlignedArray<T>::setBaseAlignment(size_type newAlign)
{
	X_ASSERT(X_ALIGN_OF(T) <= newAlign, "custom base alignment but be a multiple of type alignment")(X_ALIGN_OF(T), newAlign);
	X_ASSERT(isEmpty(), "array must be empty before changing base alignment")(isEmpty());

	align_ = newAlign;
}

template <typename T>
X_INLINE typename AlignedArray<T>::size_type AlignedArray<T>::baseAlignment(void) const
{
	return align_;
}

// ---------------------------------------------------------


// Inserts or erases elements at the end such that size is 'size'
template<typename T>
X_INLINE void AlignedArray<T>::resize(size_type newNum, const T& t)
{
	X_ASSERT(newNum >= 0, "array size must be positive")(newNum);

	size_type	i;

	// same amount of items?
	if (newNum == num_)
		return;

	// remove some?
	if (newNum < num_)
	{
		// we don't delete memory just deconstruct.
		Mem::DestructArray<T>(&list_[newNum], num_ - newNum);
	}
	else
	{
		// adding items.
		// do we have room?
		ensureSize(newNum);

		// construct the new items.
		for (i = num_; i < newNum; i++)
			Mem::Construct<T>(&list_[i], t);
	}

	// set num
	num_ = newNum;
}


// --------------------------------------------------


template<typename T>
X_INLINE void AlignedArray<T>::reserve(size_type __size)
{
	X_ASSERT(__size >= 0, "array size must be positive")(__size);
	ensureSize(__size);
}


// ---------------------------------------------------------

template<typename T>
X_INLINE typename AlignedArray<T>::Type& AlignedArray<T>::AddOne(void)
{
	if (!list_)
		reserve(granularity_);
	// grow if needs be.
	if (num_ == size_)
		reserve(size_ + granularity_);

	Mem::Construct<T>(&list_[num_]);

	return list_[num_++];
}

// ---------------------------------------------------------


template<typename T>
X_INLINE typename AlignedArray<T>::size_type AlignedArray<T>::append(T const& obj) {
	// if list empty allocate it
	if (!list_)
		reserve(granularity_);
	// grow if needs be.
	if (num_ == size_)
		reserve(size_ + granularity_);

	Mem::Construct(&list_[num_], obj);
	num_++;
	return num_ - 1;
}

template<typename T>
X_INLINE typename AlignedArray<T>::size_type AlignedArray<T>::append(T&& obj) {
	// if list empty allocate it
	if (!list_)
		reserve(granularity_);
	// grow if needs be.
	if (num_ == size_)
		reserve(size_ + granularity_);

	Mem::Construct(&list_[num_], std::forward<T>(obj));
	num_++;
	return num_ - 1;
}

template<typename T>
X_INLINE typename AlignedArray<T>::size_type AlignedArray<T>::append(const AlignedArray<T>& oth)
{
	if (this != &oth)
	{
		if ((num_ + oth.num_) > size_) {
			reserve(num_ + oth.num_);
		}

		// copy them.
		size_t i;
		for (i = 0; i < oth.num_; i++)
		{
			Mem::Construct(&list_[num_ + i], oth.list_[i]);
		}

		num_ += oth.num_;
	}

	return num_ - 1;
}

template<typename T>
X_INLINE typename AlignedArray<T>::size_type AlignedArray<T>::push_back(T const& obj)
{
	// if list empty allocate it
	if (!list_) {
		reserve(granularity_);
	}
	// grow if needs be.
	if (num_ == size_) {
		reserve(size_ + granularity_);
	}

	Mem::Construct(&list_[num_], obj);
	num_++;
	return num_ - 1;
}

template<typename T>
X_INLINE typename AlignedArray<T>::size_type AlignedArray<T>::push_back(T&& obj)
{
	// if list empty allocate it
	if (!list_) {
		reserve(granularity_);
	}
	// grow if needs be.
	if (num_ == size_) {
		reserve(size_ + granularity_);
	}

	Mem::Construct(&list_[num_], std::forward<T>(obj));
	num_++;
	return num_ - 1;
}

template<typename T>
template<class... ArgsT>
X_INLINE typename AlignedArray<T>::size_type AlignedArray<T>::emplace_back(ArgsT&&... args)
{
	if (!list_) {
		reserve(granularity_);
	}

	if (num_ == size_) {
		reserve(size_ + granularity_);
	}

	Mem::Construct<T>(&list_[num_], std::forward<ArgsT>(args)...);

	num_++;
	return num_ - 1;
}

// -----------------------------------------------

template<typename T>
X_INLINE void AlignedArray<T>::pop_back()
{
	if (size() > 0)
	{
		Mem::Destruct(end() - 1);
		num_--;
	}
}

// -----------------------------------------------

template<typename T>
X_INLINE typename AlignedArray<T>::size_type AlignedArray<T>::insert(const Type& obj, size_type index)
{
	if (!list_) {
		reserve(granularity_);
	}

	if (num_ == size_) {
		size_type newsize = size_ + granularity_;

		reserve(newsize);
	}

	if (index < 0) {
		index = 0;
	}
	else if (index > num_) {
		index = num_;
	}

	for (size_type i = num_; i > index; --i) {
		list_[i] = list_[i - 1];
	}

	num_++;
	list_[index] = obj;

	return index;
}

template<typename T>
X_INLINE typename AlignedArray<T>::size_type AlignedArray<T>::insert(Type&& obj, size_type index)
{
	if (!list_) {
		reserve(granularity_);
	}

	if (num_ == size_) {
		size_type newsize = size_ + granularity_;
		reserve(newsize);
	}

	if (index < 0) {
		index = 0;
	}
	else if (index > num_) {
		index = num_;
	}

	for (size_type i = num_; i > index; --i) {
		list_[i] = list_[i - 1];
	}

	num_++;
	Mem::Construct(&list_[index], std::forward<T>(obj));

	return index;
}


template<typename T>
bool AlignedArray<T>::removeIndex(size_type idx)
{
	if (idx == (size_type)-1) {
		return false;
	}

	X_ASSERT_NOT_NULL(list_);
	X_ASSERT(idx >= 0, "index is invalid")(idx);
	X_ASSERT(idx < num_, "index is out of bounds")(idx, num_);

	if ((idx < 0) || (idx >= num_)) {
		return false;
	}

	T* pItem = &list_[idx];

	Mem::Destruct(pItem);

	num_--;

	const bool itemsLeft = (num_ > 0);
	const bool isLast = (idx == num_);

	if (itemsLeft && !isLast)
	{
		// move end to idx we removed.
		Mem::Construct<T>(pItem, list_[num_]);
		Mem::Destruct(&list_[num_]);
	}

	return true;
}

template<typename T>
void AlignedArray<T>::remove(const T& item)
{
	size_type idx = find(item);

	if (idx != invalid_index) {
		removeIndex(idx);
		return;
	}

	X_ASSERT(false, "Item to remove could not be found.")(item);
}

template<typename T>
typename AlignedArray<T>::size_type AlignedArray<T>::find(const Type& val) const
{
	size_type i;
	size_type num = num_;

	for (i = 0; i < num; i++)
	{
		if (list_[i] == val) {
			return i;
		}
	}

	return invalid_index;
}

template<typename T>
void AlignedArray<T>::swap(AlignedArray& oth)
{
	// swap them baby.
	core::Swap(list_, oth.list_);
	core::Swap(num_, oth.num_);
	core::Swap(size_, oth.size_);
	core::Swap(granularity_, oth.granularity_);

	core::Swap(arena_, oth.arena_);
}


// -----------------------------------------------

template<typename T>
inline typename AlignedArray<T>::Iterator AlignedArray<T>::begin(void)
{
	return list_;
}

template<typename T>
inline typename AlignedArray<T>::ConstIterator AlignedArray<T>::begin(void) const
{
	return list_;
}

template<typename T>
inline typename AlignedArray<T>::Iterator AlignedArray<T>::end(void)
{
	return list_ + num_;
}

template<typename T>
inline typename AlignedArray<T>::ConstIterator AlignedArray<T>::end(void) const
{
	return list_ + num_;
}


template<typename T>
inline typename AlignedArray<T>::Reference AlignedArray<T>::front(void)
{
	X_ASSERT(isNotEmpty(), "Array can't be empty when calling front")(isNotEmpty());
	return *list_;
}

template<typename T>
inline typename AlignedArray<T>::ConstReference AlignedArray<T>::front(void) const
{
	X_ASSERT(isNotEmpty(), "Array can't be empty when calling front")(isNotEmpty());
	return *list_;
}


template<typename T>
inline typename AlignedArray<T>::Reference AlignedArray<T>::back(void)
{
	X_ASSERT(isNotEmpty(), "Array can't be empty when calling back")(isNotEmpty());
	return (*(end() - 1));
}

template<typename T>
inline typename AlignedArray<T>::ConstReference AlignedArray<T>::back(void) const
{
	X_ASSERT(isNotEmpty(), "Array can't be empty when calling back")(isNotEmpty());
	return (*(end() - 1));
}


// -----------------------------------------------

// ISerialize
template<typename T>
bool AlignedArray<T>::SSave(XFile* pFile) const
{
	X_ASSERT_NOT_NULL(pFile);

	X_DISABLE_WARNING(4127)
		if (!compileTime::IsPOD<T>::Value) {
			X_ERROR("Array", "Can't save a none POD type to file: %s", typeid(T).name());
			return false;
		}
	X_ENABLE_WARNING(4127)

		pFile->writeObj(num_);
	pFile->writeObj(size_);
	pFile->writeObj(granularity_);
	pFile->writeObj(list_, num_);
	return true;
}

template<typename T>
bool AlignedArray<T>::SLoad(XFile* pFile)
{
	X_ASSERT_NOT_NULL(pFile);

	X_DISABLE_WARNING(4127)
		if (!compileTime::IsPOD<T>::Value) {
			X_ERROR("Array", "Can't load a none POD type from file: %s", typeid(T).name());
			return false;
		}
	X_ENABLE_WARNING(4127)

		free();
	size_type read, num, size, gran;

	read = 0;
	read += pFile->readObj(num);
	read += pFile->readObj(size);
	read += pFile->readObj(gran);
	if (read != (sizeof(size_type) * 3))
	{
		X_ERROR("Array", "failed to read size info from file");
		return false;
	}

	granularity_ = gran;
	list_ = Allocate(size);
	num_ = num;
	size_ = size;

	return pFile->readObj(list_, num_) == (num_ * sizeof(T));
}


// ~ISerialize

// -----------------------------------------------


template<typename T>
void AlignedArray<T>::ensureSize(size_type size)
{
	if (size > size_)
	{
		Type* pOldList;
		size_type	i;
		size_type	newsize;

		newsize = bitUtil::RoundUpToMultiple(size, granularity_);
		pOldList = list_;

		// new array baby!
		list_ = Allocate(newsize);

		// copy old items over.
		if (pOldList)
		{
			for (i = 0; i < num_; i++)
				Mem::Construct(&list_[i], pOldList[i]);

			// delete old.
			DeleteArr(pOldList);
		}

		// set the rounded size
		size_ = newsize;
	}
}


template<typename T>
X_INLINE void AlignedArray<T>::DeleteArr(T* pArr)
{
	X_ASSERT_NOT_NULL(arena_);

	arena_->free(pArr);
	// X_DELETE_ARRAY(pArr, arena_);
}

template<typename T>
X_INLINE T* AlignedArray<T>::Allocate(size_type num)
{
	X_ASSERT_NOT_NULL(arena_);

	// we don't allocate the object type.
	// since we don't want to construct any of them.
	// that is done on a per item bases.
	return static_cast<T*>(arena_->allocate(sizeof(T)*num, align_, 0, "Array", "T[]", X_SOURCE_INFO));
}