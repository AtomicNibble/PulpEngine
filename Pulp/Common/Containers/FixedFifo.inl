

template<typename T, size_t N>
FixedFifo<T, N>::FixedFifo() :
    read_(reinterpret_cast<T*>(array_)),
    write_(reinterpret_cast<T*>(array_)),
    num_(0)
{
}

template<typename T, size_t N>
FixedFifo<T, N>::~FixedFifo()
{
}

template<typename T, size_t N>
X_INLINE T& FixedFifo<T, N>::operator[](size_type idx)
{
    X_ASSERT(idx < size(), "Index out of range.")(idx, size());

    if (read_ + idx >= endPtr()) {
        size_type left = endPtr() - read_;
        return *(startPtr() + (idx - left));
    }

    return *(read_ + idx);
}

template<typename T, size_t N>
X_INLINE const T& FixedFifo<T, N>::operator[](size_type idx) const
{
    X_ASSERT(idx < size(), "Index out of range.")(idx, size());

    if (read_ + idx >= endPtr()) {
        size_type left = endPtr() - read_;
        return *(startPtr() + (idx - left));
    }

    return *(read_ + idx);
}

template<typename T, size_t N>
void FixedFifo<T, N>::push(const T& v)
{
    X_ASSERT(size() < capacity(), "Cannot push another value into an already full FIFO.")(size(), capacity());

    Mem::Construct<T>(write_, v);

    ++write_;

    if (write_ == endPtr()) {
        write_ = reinterpret_cast<T*>(array_);
    }

    ++num_;
}

template<typename T, size_t N>
void FixedFifo<T, N>::push(T&& v)
{
    X_ASSERT(size() < capacity(), "Cannot push another value into an already full FIFO.")(size(), capacity());

    Mem::Construct<T>(write_, std::forward<T>(v));

    ++write_;

    if (write_ == endPtr()) {
        write_ = reinterpret_cast<T*>(array_);
    }

    ++num_;
}

template<typename T, size_t N>
template<class... ArgsT>
void FixedFifo<T, N>::emplace(ArgsT&&... args)
{
    X_ASSERT(size() < capacity(), "Cannot push another value into an already full FIFO.")(size(), capacity());

    Mem::Construct<T>(write_, std::forward<ArgsT>(args)...);

    ++write_;

    if (write_ == endPtr()) {
        write_ = reinterpret_cast<T*>(array_);
    }

    ++num_;
}

template<typename T, size_t N>
void FixedFifo<T, N>::pop(void)
{
    X_ASSERT(!isEmpty(), "Cannot pop value of an empty FIFO.")(size(), capacity());

    Mem::Destruct<T>(read_);

    ++read_;

    if (read_ == endPtr()) {
        read_ = reinterpret_cast<T*>(array_);
    }

    --num_;
}

template<typename T, size_t N>
T& FixedFifo<T, N>::peek(void)
{
    X_ASSERT(!isEmpty(), "Cannot access the frontmost value of an empty FIFO.")(size(), capacity());
    return *read_;
}

template<typename T, size_t N>
const T& FixedFifo<T, N>::peek(void) const
{
    X_ASSERT(!isEmpty(), "Cannot access the frontmost value of an empty FIFO.")(size(), capacity());
    return *read_;
}

template<typename T, size_t N>
void FixedFifo<T, N>::clear(void)
{
    while (size() > 0) {
        pop();
    }

    num_ = 0;
    read_ = reinterpret_cast<T*>(array_);
    write_ = reinterpret_cast<T*>(array_);
}

template<typename T, size_t N>
typename FixedFifo<T, N>::size_type FixedFifo<T, N>::size(void) const
{
    return num_;
}

template<typename T, size_t N>
typename FixedFifo<T, N>::size_type FixedFifo<T, N>::capacity(void) const
{
    return union_cast<size_type>(endPtr() - reinterpret_cast<const T*>(array_));
}

template<typename T, size_t N>
typename FixedFifo<T, N>::size_type FixedFifo<T, N>::freeSpace(void) const
{
    return capacity() - size();
}

template<typename T, size_t N>
bool FixedFifo<T, N>::isEmpty(void) const
{
    return num_ == 0;
}

template<typename T, size_t N>
bool FixedFifo<T, N>::isNotEmpty(void) const
{
    return num_ > 0;
}

// STL iterators.
template<typename T, size_t N>
typename FixedFifo<T, N>::iterator FixedFifo<T, N>::begin(void)
{
    return iterator(reinterpret_cast<T*>(array_), endPtr(), read_, 0);
}

template<typename T, size_t N>
typename FixedFifo<T, N>::iterator FixedFifo<T, N>::end(void)
{
    return iterator(reinterpret_cast<T*>(array_), endPtr(), write_, num_);
}

template<typename T, size_t N>
typename FixedFifo<T, N>::const_iterator FixedFifo<T, N>::begin(void) const
{
    return const_iterator(reinterpret_cast<const T*>(array_), endPtr(), read_, 0);
}

template<typename T, size_t N>
typename FixedFifo<T, N>::const_iterator FixedFifo<T, N>::end(void) const
{
    return const_iterator(reinterpret_cast<const T*>(array_), endPtr(), write_, num_);
}

/// ------------------------------------------------------

template<typename T, size_t N>
typename FixedFifo<T, N>::Reference FixedFifo<T, N>::front(void)
{
    X_ASSERT(isNotEmpty(), "FiFo can't be empty when calling front")(isNotEmpty());

    return *read_;
}

template<typename T, size_t N>
typename FixedFifo<T, N>::ConstReference FixedFifo<T, N>::front(void) const
{
    X_ASSERT(isNotEmpty(), "FiFo can't be empty when calling front")(isNotEmpty());

    return *read_;
}

template<typename T, size_t N>
typename FixedFifo<T, N>::Reference FixedFifo<T, N>::back(void)
{
    X_ASSERT(isNotEmpty(), "FiFo can't be empty when calling back")(isNotEmpty());

    return *(write_ - 1);
}

template<typename T, size_t N>
typename FixedFifo<T, N>::ConstReference FixedFifo<T, N>::back(void) const
{
    X_ASSERT(isNotEmpty(), "FiFo can't be empty when calling back")(isNotEmpty());

    return *(write_ - 1);
}

template<typename T, size_t N>
X_INLINE T* FixedFifo<T, N>::startPtr(void)
{
    return reinterpret_cast<T*>(array_);
}

template<typename T, size_t N>
X_INLINE const T* FixedFifo<T, N>::startPtr(void) const
{
    return reinterpret_cast<const T*>(array_);
}

template<typename T, size_t N>
X_INLINE T* FixedFifo<T, N>::endPtr(void)
{
    return reinterpret_cast<T*>(array_) + N;
}

template<typename T, size_t N>
X_INLINE const T* FixedFifo<T, N>::endPtr(void) const
{
    return reinterpret_cast<const T*>(array_) + N;
}

/// ------------------------------------------------------

template<typename T, size_t N>
inline const T& FixedFifoIterator<T, N>::operator*(void)const
{
    return *current_;
}

template<typename T, size_t N>
inline const T* FixedFifoIterator<T, N>::operator->(void)const
{
    return current_;
}

template<typename T, size_t N>
inline FixedFifoIterator<T, N>& FixedFifoIterator<T, N>::operator++(void)
{
    ++count_;
    ++current_;
    if (current_ == end_) {
        current_ = start_;
    }

    return *this;
}

template<typename T, size_t N>
inline FixedFifoIterator<T, N> FixedFifoIterator<T, N>::operator++(int)
{
    FixedFifoIterator<T, N> tmp = *this;
    ++(*this); // call the function above.
    return tmp;
}

template<typename T, size_t N>
inline bool FixedFifoIterator<T, N>::operator==(const FixedFifoIterator& rhs) const
{
    return count_ == rhs.count_;
}

template<typename T, size_t N>
inline bool FixedFifoIterator<T, N>::operator!=(const FixedFifoIterator& rhs) const
{
    return count_ != rhs.count_;
}

/// ------------------------------------------------------

template<typename T, size_t N>
inline const T& FixedFifoConstIterator<T, N>::operator*(void)const
{
    return *current_;
}

template<typename T, size_t N>
inline const T* FixedFifoConstIterator<T, N>::operator->(void)const
{
    return current_;
}

template<typename T, size_t N>
inline FixedFifoConstIterator<T, N>& FixedFifoConstIterator<T, N>::operator++(void)
{
    ++count_;
    ++current_;
    if (current_ == end_) {
        current_ = start_;
    }

    return *this;
}

template<typename T, size_t N>
inline FixedFifoConstIterator<T, N> FixedFifoConstIterator<T, N>::operator++(int)
{
    FixedFifoConstIterator tmp = *this;
    ++(*this); // call the function above.
    return tmp;
}

template<typename T, size_t N>
inline bool FixedFifoConstIterator<T, N>::operator==(const FixedFifoConstIterator& rhs) const
{
    return count_ == rhs.count_;
}

template<typename T, size_t N>
inline bool FixedFifoConstIterator<T, N>::operator!=(const FixedFifoConstIterator& rhs) const
{
    return count_ != rhs.count_;
}
