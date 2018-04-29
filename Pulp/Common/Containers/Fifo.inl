

template<typename T>
Fifo<T>::Fifo(MemoryArenaBase* arena) :
    start_(nullptr),
    end_(nullptr),
    read_(nullptr),
    write_(nullptr),
    arena_(arena)
{
}

template<typename T>
Fifo<T>::Fifo(MemoryArenaBase* arena, size_type size) :
    start_(nullptr),
    end_(nullptr),
    read_(nullptr),
    write_(nullptr),
    arena_(arena)
{
    X_ASSERT_NOT_NULL(arena);
    reserve(size);
}

template<typename T>
Fifo<T>::Fifo(const Fifo& oth) :
    Fifo<T>(oth.arena_, oth.capacity())
{
    // support nonePod
    Mem::CopyArrayUninitialized(start_, oth.start_, oth.start_ + oth.size());

    // set read/write
    read_ = start_ + (oth.read_ - oth.start_);
    write_ = start_ + (oth.write_ - oth.start_);
}

template<typename T>
Fifo<T>::Fifo(Fifo&& oth)
{
    start_ = oth.start_;
    end_ = oth.end_;
    read_ = oth.read_;
    write_ = oth.write_;

    arena_ = oth.arena_;

    // clear oth.
    oth.start_ = nullptr;
    oth.end_ = nullptr;
    oth.read_ = nullptr;
    oth.write_ = nullptr;
}

template<typename T>
Fifo<T>::~Fifo()
{
    free();
}

template<typename T>
Fifo<T>& Fifo<T>::operator=(const Fifo<T>& oth)
{
    if (this != &oth) {
        free();

        arena_ = oth.arena_;

        reserve(oth.capacity());

        Mem::CopyArrayUninitialized(start_, oth.start_, oth.start_ + oth.size());

        // set read/write
        read_ = start_ + (oth.read_ - oth.start_);
        write_ = start_ + (oth.write_ - oth.start_);
    }
    return *this;
}

template<typename T>
Fifo<T>& Fifo<T>::operator=(Fifo<T>&& oth)
{
    if (this != &oth) {
        free();

        start_ = oth.start_;
        end_ = oth.end_;
        read_ = oth.read_;
        write_ = oth.write_;

        arena_ = oth.arena_;

        // clear oth.
        oth.start_ = nullptr;
        oth.end_ = nullptr;
        oth.read_ = nullptr;
        oth.write_ = nullptr;
    }
    return *this;
}

template<typename T>
void Fifo<T>::setArena(MemoryArenaBase* arena)
{
    X_ASSERT(arena_ == nullptr || size() == 0, "can't set arena on a Fifo that has items")(size());
    arena_ = arena;
}

template<typename T>
X_INLINE T& Fifo<T>::operator[](size_type idx)
{
    X_ASSERT(idx < size(), "Index out of range.")(idx, size());

    if (read_ + idx >= end_) {
        size_type left = end_ - read_;
        return *(start_ + (idx - left));
    }

    return *(read_ + idx);
}

template<typename T>
X_INLINE const T& Fifo<T>::operator[](size_type idx) const
{
    X_ASSERT(idx < size(), "Index out of range.")(idx, size());

    if (read_ + idx >= end_) {
        size_type left = end_ - read_;
        return *(start_ + (idx - left));
    }

    return *(read_ + idx);
}

template<typename T>
void Fifo<T>::push(const T& v)
{
    if (!start_) {
        expand();
    }

    Mem::Construct<T>(write_, v);

    ++write_;

    if (write_ == end_) {
        write_ = start_;
    }

    if (write_ == read_) {
        expand();
    }
}

template<typename T>
void Fifo<T>::push(T&& v)
{
    if (!start_) {
        expand();
    }

    Mem::Construct<T>(write_, std::forward<T>(v));

    ++write_;

    if (write_ == end_) {
        write_ = start_;
    }

    if (write_ == read_) {
        expand();
    }
}

template<typename T>
template<class... ArgsT>
void Fifo<T>::emplace(ArgsT&&... args)
{
    if (!start_) {
        expand();
    }

    Mem::Construct<T>(write_, std::forward<ArgsT>(args)...);

    ++write_;

    if (write_ == end_) {
        write_ = start_;
    }

    if (write_ == read_) {
        expand();
    }
}

template<typename T>
void Fifo<T>::pop(void)
{
    X_ASSERT(!isEmpty(), "Cannot pop value of an empty FIFO.")(size(), capacity());

    Mem::Destruct<T>(read_);

    ++read_;

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
    for (auto it = begin(); it != endIt; ++it) {
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
    for (auto it = begin(); it != endIt; ++it) {
        if (p(*it)) {
            return true;
        }
    }

    return false;
}

template<typename T>
void Fifo<T>::reserve(size_type num)
{
    X_ASSERT(start_ == nullptr, "Cannot reserve additional memory. free() the FIFO first.")(size(), capacity(), start_, end_, num);

    start_ = Allocate(num);
    end_ = start_ + num;
    read_ = start_;
    write_ = start_;
}

template<typename T>
void Fifo<T>::clear(void)
{
    size_type num = size();
    for (size_type i = 0; i < num; i++) {
        pop();
    }

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
void Fifo<T>::shrinkToFit(void)
{
    if (capacity() > size() + 1) {
        // reallocate.
        size_type curSize = size();
        size_type newSize = size() + 1;
        T* pData = Allocate(newSize);

        // move to new memory.
        if (!isWrapped()) {
            // no wrap.
            X_ASSERT(newSize > union_cast<size_type>(write_ - read_), "Out of range")(newSize, union_cast<size_type>(write_ - read_));
            Mem::MoveArrayDestructUninitialized(pData, read_, write_);
        }
        else {
            // wrap.
            Mem::MoveArrayDestructUninitialized(pData, read_, end_);
            Mem::MoveArrayDestructUninitialized(pData + (end_ - read_), start_, write_);
        }

        // delete old and update pointers.
        Delete(start_);

        start_ = pData;
        end_ = pData + newSize;
        read_ = start_;
        write_ = read_ + curSize;

        X_ASSERT(size() == curSize, "Size has changed")(size(), curSize);
    }
}

template<typename T>
typename Fifo<T>::size_type Fifo<T>::size(void) const
{
    if (read_ <= write_) {
        X_ASSERT(!isWrapped(), "Should not be wrapped")(isWrapped(), read_, write_, start_, end_);
        return write_ - read_;
    }

    X_ASSERT(isWrapped(), "Should be wrapped")(isWrapped(), read_, write_, start_, end_);

    // looped back. so write is lower than read
    return capacity() - (read_ - write_);
}

template<typename T>
typename Fifo<T>::size_type Fifo<T>::capacity(void) const
{
    return end_ - start_;
}

template<typename T>
typename Fifo<T>::size_type Fifo<T>::freeSpace(void) const
{
    return capacity() - size();
}

template<typename T>
bool Fifo<T>::isEmpty(void) const
{
    return read_ == write_ || (read_ == end_ && write_ == start_);
}

template<typename T>
bool Fifo<T>::isNotEmpty(void) const
{
    return !isEmpty();
}

template<typename T>
bool Fifo<T>::isWrapped(void) const
{
    // if read is equal or less to write we are not wrapped.
    // we include equal since a empty buffer can't be wrapped.
    return read_ > write_;
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
    return iterator(start_, end_, write_, size());
}

template<typename T>
typename Fifo<T>::const_iterator Fifo<T>::begin(void) const
{
    return const_iterator(start_, end_, read_, 0);
}

template<typename T>
typename Fifo<T>::const_iterator Fifo<T>::end(void) const
{
    return const_iterator(start_, end_, write_, size());
}

/// ------------------------------------------------------c

template<typename T>
typename Fifo<T>::Reference Fifo<T>::front(void)
{
    X_ASSERT(isNotEmpty(), "FiFo can't be empty when calling front")(isNotEmpty());

    return *read_;
}

template<typename T>
typename Fifo<T>::ConstReference Fifo<T>::front(void) const
{
    X_ASSERT(isNotEmpty(), "FiFo can't be empty when calling front")(isNotEmpty());

    return *read_;
}

template<typename T>
typename Fifo<T>::Reference Fifo<T>::back(void)
{
    X_ASSERT(isNotEmpty(), "FiFo can't be empty when calling back")(isNotEmpty());

    TypePtr pTr = write_ - 1;
    if (write_ == start_) {
        pTr = end_ - 1;
    }

    X_ASSERT(pTr >= start_ && pTr < end_, "Out of range")(pTr, start_, end_, size(), capacity());
    return *pTr;
}

template<typename T>
typename Fifo<T>::ConstReference Fifo<T>::back(void) const
{
    X_ASSERT(isNotEmpty(), "FiFo can't be empty when calling back")(isNotEmpty());

    ConstTypePtr pTr = write_ - 1;
    if (write_ == start_) {
        pTr = end_ - 1;
    }

    X_ASSERT(pTr >= start_ && pTr < end_, "Out of range")(pTr, start_, end_, size(), capacity());
    return *pTr;
}

/// ----------------------------

template<typename T>
void Fifo<T>::expand(void)
{
    X_ASSERT(write_ == read_, "This should only be called when we are full.")(write_, read_);

    // we want to allocate a new aaray like a slut.
    // if first time jump to 16.
    if (capacity() == 0) {
        reserve(16);
        return;
    }

    X_ASSERT_NOT_NULL(start_);

    size_type curSize = capacity();
    size_type newSize = capacity() << 1;
    T* pData = Allocate(newSize);

    // move to new memory.
    Mem::MoveArrayDestructUninitialized(pData, read_, end_);
    // handle wrap around.
    Mem::MoveArrayDestructUninitialized(pData + (end_ - read_), start_, write_);

    // delete old and update pointers.
    Delete(start_);

    start_ = pData;
    end_ = pData + newSize;
    read_ = start_; // read always moves back to start, when we grow.
    write_ = read_ + curSize;
}

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
    return reinterpret_cast<T*>(X_NEW_ARRAY(uint8_t, num * sizeof(T), arena_, "Fifo<" X_PP_STRINGIZE(T) ">"));
}

/// ------------------------------------------------------c

template<typename T>
inline const T& FifoIterator<T>::operator*(void)const
{
    return *current_;
}

template<typename T>
inline const T* FifoIterator<T>::operator->(void)const
{
    return current_;
}

template<typename T>
inline FifoIterator<T>& FifoIterator<T>::operator++(void)
{
    ++count_;
    ++current_;
    if (current_ == end_) {
        current_ = start_;
    }

    return *this;
}

template<typename T>
inline FifoIterator<T> FifoIterator<T>::operator++(int)
{
    FifoIterator<T> tmp = *this;
    ++(*this); // call the function above.
    return tmp;
}

template<typename T>
inline bool FifoIterator<T>::operator==(const FifoIterator& rhs) const
{
    return current_ == rhs.current_;
}

template<typename T>
inline bool FifoIterator<T>::operator!=(const FifoIterator& rhs) const
{
    return current_ != rhs.current_;
}

/// ------------------------------------------------------

template<typename T>
inline const T& FifoConstIterator<T>::operator*(void)const
{
    return *current_;
}

template<typename T>
inline const T* FifoConstIterator<T>::operator->(void)const
{
    return current_;
}

template<typename T>
inline FifoConstIterator<T>& FifoConstIterator<T>::operator++(void)
{
    ++count_;
    ++current_;
    if (current_ == end_) {
        current_ = start_;
    }

    return *this;
}

template<typename T>
inline FifoConstIterator<T> FifoConstIterator<T>::operator++(int)
{
    FifoConstIterator<T> tmp = *this;
    ++(*this); // call the function above.
    return tmp;
}

template<typename T>
inline bool FifoConstIterator<T>::operator==(const FifoConstIterator<T>& rhs) const
{
    return current_ == rhs.current_;
}

template<typename T>
inline bool FifoConstIterator<T>::operator!=(const FifoConstIterator<T>& rhs) const
{
    return current_ != rhs.current_;
}
