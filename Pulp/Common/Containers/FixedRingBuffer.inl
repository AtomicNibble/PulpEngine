
X_NAMESPACE_BEGIN(core)

template<typename T, size_t N>
FixedRingBuffer<T, N>::FixedRingBuffer() :
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
        Mem::Destruct<T>(data() + indexToSubscript(i));
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
    size_type next = nextTail();
    if (num_ == N) {
        data()[next] = val;
        incrementHead();
    }
    else {
        Mem::Construct<T>(data() + next, val);
    }

    incrementTail();
}

template<typename T, size_t N>
void FixedRingBuffer<T, N>::push_back(const T& val)
{
    append(val);
}

template<typename T, size_t N>
template<class... ArgsT>
void FixedRingBuffer<T, N>::emplace_back(ArgsT&&... args)
{
    size_type next = nextTail();
    if (num_ == N) {
        Mem::Destruct<T>(data() + next);
        Mem::Construct<T>(data() + next, std::forward<ArgsT>(args)...);

        incrementHead();
    }
    else {
        Mem::Construct<T>(data() + next, std::forward<ArgsT>(args)...);
    }

    incrementTail();
}

// -----------------------------------

template<typename T, size_t N>
typename FixedRingBuffer<T, N>::iterator FixedRingBuffer<T, N>::begin(void)
{
    return iterator(this, 0);
}

template<typename T, size_t N>
typename FixedRingBuffer<T, N>::iterator FixedRingBuffer<T, N>::end(void)
{
    return iterator(this, size());
}

template<typename T, size_t N>
typename FixedRingBuffer<T, N>::const_iterator FixedRingBuffer<T, N>::begin(void) const
{
    return const_iterator(this, 0);
}

template<typename T, size_t N>
typename FixedRingBuffer<T, N>::const_iterator FixedRingBuffer<T, N>::end(void) const
{
    return const_iterator(this, size());
}

// -----------------------------------

template<typename T, size_t N>
typename FixedRingBuffer<T, N>::reverse_iterator FixedRingBuffer<T, N>::rbegin(void)
{
    return reverse_iterator(end());
}

template<typename T, size_t N>
typename FixedRingBuffer<T, N>::reverse_iterator FixedRingBuffer<T, N>::rend(void)
{
    return reverse_iterator(begin());
}

template<typename T, size_t N>
typename FixedRingBuffer<T, N>::const_reverse_iterator FixedRingBuffer<T, N>::rbegin(void) const
{
    return const_reverse_iterator(end());
}

template<typename T, size_t N>
typename FixedRingBuffer<T, N>::const_reverse_iterator FixedRingBuffer<T, N>::rend(void) const
{
    return const_reverse_iterator(begin());
}

// -----------------------------------

template<typename T, size_t N>
typename FixedRingBuffer<T, N>::reference FixedRingBuffer<T, N>::operator[](size_type idx)
{
    return data()[indexToSubscript(idx)];
}

template<typename T, size_t N>
typename FixedRingBuffer<T, N>::const_reference FixedRingBuffer<T, N>::operator[](size_type idx) const
{
    return data()[indexToSubscript(idx)];
}

// -----------------------------------
template<typename T, size_t N>
typename FixedRingBuffer<T, N>::size_type FixedRingBuffer<T, N>::normalise(size_type idx) const
{
    return idx % N;
}

template<typename T, size_t N>
typename FixedRingBuffer<T, N>::size_type FixedRingBuffer<T, N>::indexToSubscript(size_type idx) const
{
    return normalise(idx + head_);
}

template<typename T, size_t N>
typename FixedRingBuffer<T, N>::size_type FixedRingBuffer<T, N>::nextTail(void)
{
    return (tail_ + 1 == N) ? 0 : tail_ + 1;
}

template<typename T, size_t N>
void FixedRingBuffer<T, N>::incrementTail(void)
{
    ++num_;
    tail_ = nextTail();
}

template<typename T, size_t N>
void FixedRingBuffer<T, N>::incrementHead(void)
{
    ++head_;
    --num_;
    if (head_ == N) {
        head_ = 0;
    }
}

template<typename T, size_t N>
T* FixedRingBuffer<T, N>::data(void)
{
    return reinterpret_cast<T*>(array_);
}

X_NAMESPACE_END
