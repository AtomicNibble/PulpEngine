
template<typename T, class Allocator>
X_INLINE Array<T, Allocator>::Array(MemoryArenaBase* arena) :
	list_( nullptr ),
	num_( 0 ),
	size_( 0 ),
	granularity_( 16 ),
	allocator_(arena)
{
}


template<typename T, class Allocator>
X_INLINE Array<T, Allocator>::Array(MemoryArenaBase* arena, size_type size) :
	list_(nullptr),
	num_(size),
	size_(size),
	granularity_(16),
	allocator_(X_ASSERT_NOT_NULL(arena))
{
	if (size)
	{
		list_ = Allocate(size);
		Mem::ConstructArray<T>(list_, size);
	}
}

template<typename T, class Allocator>
X_INLINE Array<T, Allocator>::Array(MemoryArenaBase* arena, size_type size, const T& initialValue) :
    list_(nullptr),
    num_(size),
    size_(size),
    granularity_(16),
    allocator_(X_ASSERT_NOT_NULL(arena))
{
	if (size)
	{
		list_ = Allocate(size);
		Mem::ConstructArray(list_, size, initialValue);
	}
}

template<typename T, class Allocator>
X_INLINE Array<T, Allocator>::Array(MemoryArenaBase* arena, std::initializer_list<T> iList) :
	Array(arena)
{
	size_t size = iList.size();

	ensureSize(size);

	Mem::CopyArrayUninitialized<T>(list_, iList.begin(), iList.end());

	num_ = size;
}

template<typename T, class Allocator>
X_INLINE Array<T, Allocator>::Array(const MyT& oth) :
    list_(nullptr),
    num_(0),
    size_(0),
    granularity_(16),
    allocator_(oth.allocator_)
{
	*this = oth;
}


template<typename T, class Allocator>
X_INLINE Array<T, Allocator>::Array(Array<T, Allocator>&& oth) :
	list_(oth.list_),
	num_(oth.num_),
	size_(oth.size_),
	granularity_(oth.granularity_),
	allocator_(oth.allocator_)
{
	// clear other.
	oth.list_ = nullptr;
	oth.num_ = 0;
	oth.size_ = 0;
}


template<typename T, class Allocator>
X_INLINE Array<T, Allocator>::~Array(void) 
{
	free();
}

template<typename T, class Allocator>
X_INLINE void Array<T, Allocator>::setArena(MemoryArenaBase* arena)
{
	X_ASSERT(num_ == 0, "can't set arena on a array that has items")(num_);
	allocator_.setArena(arena);
}

template<typename T, class Allocator>
X_INLINE void Array<T, Allocator>::setArena(MemoryArenaBase* arena, size_type capacity)
{
	X_ASSERT(num_ == 0, "can't set arena on a array that has items")(num_);
	allocator_.setArena(arena);

	reserve(capacity);
}

template<typename T, class Allocator>
X_INLINE core::MemoryArenaBase* Array<T, Allocator>::getArena(void) const
{
	return allocator_.getArena();
}

// ---------------------------------------------------------

template<typename T, class Allocator>
X_INLINE Allocator& Array<T, Allocator>::getAllocator(void)
{
	return allocator_;
}

template<typename T, class Allocator>
X_INLINE const Allocator& Array<T, Allocator>::getAllocator(void) const
{
	return allocator_;
}

// ---------------------------------------------------------

template<typename T, class Allocator>
Array<T, Allocator>& Array<T, Allocator>::operator=(std::initializer_list<T> iList)
{
	free();

	size_t size = iList.size();

	if (size) {
		ensureSize(size);
		Mem::CopyArrayUninitialized(list_, iList.begin(), iList.end());
	}

	num_ = size;
	return *this;
}

template<typename T, class Allocator>
Array<T, Allocator>& Array<T, Allocator>::operator=(const MyT& oth)
{
	free();

	num_ = oth.num_;
	size_ = oth.size_;
	granularity_ = oth.granularity_;
	allocator_ = oth.allocator_;

	if (size_) {
		list_ = Allocate(size_);
		Mem::CopyArrayUninitialized(list_, oth.begin(), oth.end());
	}

	return *this;
}

template<typename T, class Allocator>
Array<T, Allocator>& Array<T, Allocator>::operator=(MyT&& oth)
{
	if (this != &oth)
	{
		free();

		list_ = oth.list_;
		num_ = oth.num_;
		size_ = oth.size_;
		granularity_ = oth.granularity_;
		allocator_ = oth.allocator_;

		// clear other.
		oth.list_ = nullptr;
		oth.num_ = 0;
		oth.size_ = 0;
	}
	return *this;
}

template<typename T, class Allocator>
X_INLINE const T&Array<T, Allocator>::operator[](size_type idx) const {
	X_ASSERT(idx >= 0 && idx < num_, "Array index out of bounds")(idx, num_);
	return list_[idx];
}


template<typename T, class Allocator>
X_INLINE T&Array<T, Allocator>::operator[](size_type idx) {
	X_ASSERT(idx >= 0 && idx < num_, "Array index out of bounds")(idx, num_);
	return list_[idx];
}

// ---------------------------------------------------------

template<typename T, class Allocator>
X_INLINE T *Array<T, Allocator>::ptr(void) {
	return list_;
}


template<typename T, class Allocator>
const X_INLINE T *Array<T, Allocator>::ptr(void) const {
	return list_;
}

template<typename T, class Allocator>
X_INLINE T *Array<T, Allocator>::data(void) {
	if (isNotEmpty()) {
		return &front();
	}

	return nullptr;
}


template<typename T, class Allocator>
const X_INLINE T *Array<T, Allocator>::data(void) const {
	if (isNotEmpty()) {
		return &front();
	}

	return nullptr;
}

// ---------------------------------------------------------

template<typename T, class Allocator>
X_INLINE const bool Array<T, Allocator>::isEmpty(void) const
{
	return num_ == 0;
}

template<typename T, class Allocator>
X_INLINE const bool Array<T, Allocator>::isNotEmpty(void) const
{
	return num_ > 0;
}

template<typename T, class Allocator>
X_INLINE void Array<T, Allocator>::clear(void) 
{
	// properly destruct the instances
	Mem::DestructArray(list_, size());

	num_ = 0; // don't free any memory
}

template<typename T, class Allocator>
X_INLINE void Array<T, Allocator>::free(void)
{
	clear(); // make sure to destruct the objects.

	if (list_) {
		DeleteArr(list_);
	}

	list_ = nullptr;
	num_ =  0;
	size_ = 0;
}

template<typename T, class Allocator>
X_INLINE void Array<T, Allocator>::shrinkToFit(void)
{
	if (capacity() > size())
	{
		Type* pOldList = list_;

		// new list, do i want to make it a multiple of gran tho?
		list_ = Allocate(num_);

		// move old items over.
		if (pOldList)
		{
			Mem::MoveArrayUninitialized(list_, pOldList, pOldList + num_);

			// delete old.
			Mem::DestructArray(pOldList, num_);

			DeleteArr(pOldList);
		}

		size_ = num_;
	}
}


template<typename T, class Allocator>
X_INLINE typename Array<T, Allocator>::size_type Array<T, Allocator>::size(void) const {
	return num_;
}


template<typename T, class Allocator>
X_INLINE typename Array<T, Allocator>::size_type Array<T, Allocator>::capacity(void) const {
	return size_;
}


template<typename T, class Allocator>
X_INLINE void Array<T, Allocator>::setGranularity(size_type newgranularity)
{
	X_ASSERT( newgranularity >= 0, "granularity size must be positive" )( newgranularity );

	granularity_ = newgranularity;
}


template<typename T, class Allocator>
X_INLINE typename Array<T, Allocator>::size_type Array<T, Allocator>::granularity(void) const {
	return granularity_;
}

// ---------------------------------------------------------


// Inserts or erases elements at the end such that size is 'size'
template<typename T, class Allocator>
X_INLINE void Array<T, Allocator>::resize(size_type newNum)
{
	X_ASSERT(newNum >= 0, "array size must be positive")(newNum);

	// same amount of items?
	if (newNum == num_) {
		return;
	}

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
		Mem::ConstructArray<T>(&list_[num_], newNum - num_);
	}

	// set num
	num_ = newNum;
}

// Inserts or erases elements at the end such that size is 'size'
template<typename T, class Allocator>
X_INLINE void Array<T, Allocator>::resize(size_type newNum, const T& t) 
{
	X_ASSERT(newNum >= 0, "array size must be positive")(newNum);

	if (newNum == num_) {
		return;
	}

	if (newNum < num_)
	{
		Mem::DestructArray<T>(&list_[newNum], num_ - newNum);
	}
	else
	{
		ensureSize(newNum);

		Mem::ConstructArray<T>(&list_[num_], newNum - num_, t);
	}

	num_ = newNum;
}


// --------------------------------------------------


template<typename T, class Allocator>
X_INLINE void Array<T, Allocator>::reserve(size_type __size) 
{
	X_ASSERT(__size >= 0, "array size must be positive")(__size);
	ensureSize(__size);
}


// ---------------------------------------------------------

template<typename T, class Allocator>
template<class... Args>
X_INLINE typename Array<T, Allocator>::Type& Array<T, Allocator>::AddOne(Args&&... args)
{
	// grow if needs be.
	if (num_ == size_) {
		ensureSize(size_ + 1);
	}

	Mem::Construct<T>(&list_[num_], std::forward<Args>(args)...);

	return list_[num_++];
}

// ---------------------------------------------------------


template<typename T, class Allocator>
X_INLINE typename Array<T, Allocator>::size_type Array<T, Allocator>::append(T const& obj)
{
	// grow if needs be.
	if (num_ == size_) {
		ensureSize(size_ + 1);
	}

	Mem::Construct(&list_[num_], obj);
	num_++;
	return num_ - 1;
}

template<typename T, class Allocator>
X_INLINE typename Array<T, Allocator>::size_type Array<T, Allocator>::append(T&& obj)
{
	// grow if needs be.
	if (num_ == size_) {
		ensureSize(size_ + 1);
	}

	Mem::Construct(&list_[num_], std::forward<T>(obj));
	num_++;
	return num_ - 1;
}

template<typename T, class Allocator>
X_INLINE typename Array<T, Allocator>::size_type Array<T, Allocator>::append(const MyT& oth)
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

template<typename T, class Allocator>
X_INLINE typename Array<T, Allocator>::size_type Array<T, Allocator>::push_back(T const& obj)
{
	// grow if needs be.
	if (num_ == size_) {
		ensureSize(size_ + 1);
	}

	Mem::Construct(&list_[num_], obj);
	num_++;
	return num_ - 1;
}

template<typename T, class Allocator>
X_INLINE typename Array<T, Allocator>::size_type Array<T, Allocator>::push_back(T&& obj)
{
	// grow if needs be.
	if (num_ == size_) {
		ensureSize(size_ + 1);
	}

	Mem::Construct(&list_[num_], std::forward<T>(obj));
	num_++;
	return num_ - 1;
}

template<typename T, class Allocator>
template<class... ArgsT>
X_INLINE typename Array<T, Allocator>::size_type Array<T, Allocator>::emplace_back(ArgsT&&... args)
{
	if (num_ == size_) {
		ensureSize(size_ + 1);
	}

	Mem::Construct<T>(&list_[num_], std::forward<ArgsT>(args)...);

	num_++;
	return num_ - 1;
}

// -----------------------------------------------

template<typename T, class Allocator>
X_INLINE void Array<T, Allocator>::pop_back()
{
	if (size() > 0)
	{
		Mem::Destruct(end() - 1);
		num_--;
	}
}

// -----------------------------------------------

template<typename T, class Allocator>
X_INLINE typename Array<T, Allocator>::size_type Array<T, Allocator>::insertAtIndex(size_type index, const Type& obj)
{
	X_ASSERT(index >= 0, "index is invalid")(index);
	X_ASSERT(index <= num_, "index is out of bounds")(index, num_);

	if (num_ == size_) {
		ensureSize(size_ + 1);
	}

	Iterator pos = begin() + index;

	// move everything up.
	Mem::Move(pos, end(), pos + 1);
	Mem::Construct(pos, obj);

	++num_;
	return index;
}

template<typename T, class Allocator>
X_INLINE typename Array<T, Allocator>::size_type Array<T, Allocator>::insertAtIndex(size_type index, Type&& obj)
{
	X_ASSERT(index >= 0, "index is invalid")(index);
	X_ASSERT(index <= num_, "index is out of bounds")(index, num_);

	if (num_ == size_) {
		ensureSize(size_ + 1);
	}

	Iterator pos = begin() + index;

	// move everything up.
	Mem::Move(pos, end(), pos + 1);
	Mem::Construct(pos, std::forward<T>(obj));

	++num_;
	return index;
}


template<typename T, class Allocator>
typename Array<T, Allocator>::Iterator Array<T, Allocator>::insert(ConstIterator pos, const Type& obj)
{
	return insert(pos, 1, obj);
}

template<typename T, class Allocator>
typename Array<T, Allocator>::Iterator Array<T, Allocator>::insert(ConstIterator pos, Type&& obj)
{
	return emplace(pos, std::move(obj));
}

template<typename T, class Allocator>
typename Array<T, Allocator>::Iterator Array<T, Allocator>::insert(ConstIterator _pos, size_type count, const Type& obj)
{
	Iterator pos = const_cast<Iterator>(_pos);

	// gow
	if (num_ == size_) 
	{
		size_type offset = pos - begin();

		ensureSize(size_ + count);

		// update pos to be valid after relocate.
		pos = begin() + offset;
	}

	if (union_cast<size_type>(end() - pos) < count)
	{
		// spills off end.
		// we need to move the ones before endto new end.
		// then assing new values.
		Mem::MoveArrayUninitialized(pos + count, pos, end());

		// tecnically we can do a uninitialized fill from old end to new pos.
		std::uninitialized_fill<Iterator, T>(end(), pos + count, obj);

		std::fill<Iterator, T>(pos, end(), obj);
	}
	else
	{
		// move before end to fresh memory, past current end.
		Mem::MoveArrayUninitialized(end(), end() - count, end());

		// move items to fill gap at end, this is moving to initialized memory.
		std::move_backward<Iterator>(pos, end() - count, end());

		// now we have a hole of pos + count to fill.
		std::fill<Iterator, T>(pos, pos + count, obj);
	}

	num_ += count;

	return pos;
}


template<typename T, class Allocator>
X_INLINE typename Array<T, Allocator>::Iterator Array<T, Allocator>::insert_sorted(const Type& obj)
{
	return insert_sorted(obj, std::less<Type>());
}

template<typename T, class Allocator>
template<class Compare>
X_INLINE typename Array<T, Allocator>::Iterator Array<T, Allocator>::insert_sorted(const Type& obj, Compare comp)
{
	auto it = std::upper_bound(begin(), end(), obj, comp);
	return insert(it, obj);
}

template<typename T, class Allocator>
bool Array<T, Allocator>::removeIndex(size_type idx)
{
	if (idx == invalid_index) {
		return false;
	}

	X_ASSERT_NOT_NULL(list_);
	X_ASSERT(idx < num_, "index is out of bounds")(idx, num_);

	T* pItem = &list_[idx];

	Mem::Destruct(pItem);

	num_--;

	const bool itemsLeft = (num_ > 0);
	const bool isLast = (idx == num_);

	if (itemsLeft && !isLast)
	{
		// move end to idx we removed.
		Mem::Construct<T>(pItem, std::move(list_[num_]));
		Mem::Destruct(&list_[num_]);
	}

	return true;
}

template<typename T, class Allocator>
void Array<T, Allocator>::remove(ConstIterator it)
{
	X_ASSERT(it >= begin() && it < end(), "Invalid iterator")(it, begin(), end());

	removeIndex(it - list_);
}

template<typename T, class Allocator>
void Array<T, Allocator>::remove(const T& item)
{
	size_type idx = find(item);
	X_ASSERT(idx != invalid_index, "Item to remove could not be found.")(item, idx);

	removeIndex(idx);
}

template<typename T, class Allocator>
typename Array<T, Allocator>::Iterator Array<T, Allocator>::erase(ConstIterator _first)
{
	X_ASSERT(_first >= begin() && _first < end(), "Invalid iterator")(_first, begin(), end());

	Iterator first = const_cast<Iterator>(_first);

	// move anything after what we are deleting down.
	Iterator ptr = Mem::Move(first + 1, end(), first);

	// now we just need to deconstruct trailing.
	Mem::Destruct(ptr);

	--num_;

	return const_cast<Iterator>(first);
}

template<typename T, class Allocator>
typename Array<T, Allocator>::Iterator Array<T, Allocator>::erase(ConstIterator _first, ConstIterator _last)
{
	if (_first == begin() && _last == end())
	{
		clear();
	}
	else if(_first != _last)
	{
		Iterator first = const_cast<Iterator>(_first);
		Iterator last = const_cast<Iterator>(_last);

		// move anything after what we are deleting down.
		Iterator ptr = Mem::Move(last, end(), first);

		// now we just need to deconstruct trailing.
		const size_type num = end() - ptr;
		Mem::DestructArray(ptr, num);

		num_ -= num;
	}

	return const_cast<Iterator>(_first);
}

template<typename T, class Allocator>
typename Array<T, Allocator>::size_type Array<T, Allocator>::find(const Type& val) const
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

template<typename T, class Allocator>
typename Array<T, Allocator>::ConstIterator Array<T, Allocator>::findSorted(const Type& val) const
{
	return findSorted(val, std::less<Type>());
}

template<typename T, class Allocator>
template<class Compare>
typename Array<T, Allocator>::ConstIterator Array<T, Allocator>::findSorted(const Type& val, Compare comp) const
{
	auto it = std::lower_bound(begin(), end(), val, comp);

	if (it != end() && !comp(val, *it)) {
		return it;
	}
	return end();
}

template<typename T, class Allocator>
template<typename KeyType, class Compare, class CompareGt>
typename Array<T, Allocator>::ConstIterator Array<T, Allocator>::findSortedKey(const KeyType& val,
	Compare comp, CompareGt compGreater) const
{
	// binary search but with a key.
	size_type count = num_;
	size_type step;
	ConstIterator it;
	ConstIterator first = begin();

	while (count > 0)
	{
		step = count / 2;
		it = first + step;

		if (comp(*it, val))
		{
			first = ++it;
			count -= step + 1;
		}
		else
		{
			count = step;
		}
	}

	// check if actuall found.
	// i can't think of way todo this without requiring one of the following addition comps:
	// left hand side value.
	// greater than.
	// equal
	if (first != end() && !compGreater(*first, val)) {
		return first;
	}

	return end();
}

template<typename T, class Allocator>
void Array<T, Allocator>::swap(Array& oth)
{
	// swap them baby.
	core::Swap(list_, oth.list_);
	core::Swap(num_, oth.num_);
	core::Swap(size_, oth.size_);
	core::Swap(granularity_, oth.granularity_);

	core::Swap(allocator_, oth.allocator_);
}


// -----------------------------------------------

template<typename T, class Allocator>
inline typename Array<T, Allocator>::Iterator Array<T, Allocator>::begin(void)
{
	return list_;
}

template<typename T, class Allocator>
inline typename Array<T, Allocator>::ConstIterator Array<T, Allocator>::begin(void) const
{
	return list_;
}

template<typename T, class Allocator>
inline typename Array<T, Allocator>::Iterator Array<T, Allocator>::end(void)
{
	return list_ + num_;
}

template<typename T, class Allocator>
inline typename Array<T, Allocator>::ConstIterator Array<T, Allocator>::end(void) const
{
	return list_ + num_;
}


template<typename T, class Allocator>
inline typename Array<T, Allocator>::Reference Array<T, Allocator>::front(void)
{
	X_ASSERT(isNotEmpty(), "Array can't be empty when calling front")(isNotEmpty());
	return *list_;
}

template<typename T, class Allocator>
inline typename Array<T, Allocator>::ConstReference Array<T, Allocator>::front(void) const
{
	X_ASSERT(isNotEmpty(), "Array can't be empty when calling front")(isNotEmpty());
	return *list_;
}


template<typename T, class Allocator>
inline typename Array<T, Allocator>::Reference Array<T, Allocator>::back(void)
{
	X_ASSERT(isNotEmpty(), "Array can't be empty when calling back")(isNotEmpty());
	return (*(end() - 1));
}

template<typename T, class Allocator>
inline typename Array<T, Allocator>::ConstReference Array<T, Allocator>::back(void) const
{
	X_ASSERT(isNotEmpty(), "Array can't be empty when calling back")(isNotEmpty());
	return (*(end() - 1));
}


// -----------------------------------------------

// ISerialize
template<typename T, class Allocator>
bool Array<T, Allocator>::SSave(XFile* pFile) const
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

template<typename T, class Allocator>
bool Array<T, Allocator>::SLoad(XFile* pFile)
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
	if (read != (sizeof(size_type)* 3))
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


template<typename T, class Allocator>
void Array<T, Allocator>::ensureSize(size_type size)
{
	if (size > size_)
	{
		Type* pOldList;
		size_type	newsize;

		newsize = bitUtil::RoundUpToMultiple(size, granularity_);
		pOldList = list_;

		// new array baby!
		list_ = Allocate(newsize);

		// move old items over.
		if (pOldList)
		{
			Mem::MoveArrayDestructUninitialized(list_, pOldList, pOldList + num_);

			// delete old.
			DeleteArr(pOldList);
		}

		// set the rounded size
		size_ = newsize;
	}
}


template<typename T, class Allocator>
X_INLINE void Array<T, Allocator>::DeleteArr(T* pArr)
{
	allocator_.free(pArr);
}

template<typename T, class Allocator>
X_INLINE T* Array<T, Allocator>::Allocate(size_type num)
{
	// we don't allocate the object type.
	// since we don't want to construct any of them.
	// that is done on a per item bases.
	return allocator_.allocate(num);
}