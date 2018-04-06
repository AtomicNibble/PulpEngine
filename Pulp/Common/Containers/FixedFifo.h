#pragma once

#ifndef _X_CON_FIXED_FIFO_H_
#define _X_CON_FIXED_FIFO_H_

X_NAMESPACE_BEGIN(core)

template<typename T, size_t N>
class FixedFifo;

template<typename T, size_t N>
struct FixedFifoIterator;

template<typename T, size_t N>
struct FixedFifoConstIterator;

template<typename T, size_t N>
struct FixedFifoIterator
{
    typedef FixedFifoConstIterator<T, N> const_iterator;

    FixedFifoIterator(T* start, T* end, T* cur, size_t count) :
        start_(start),
        end_(end),
        current_(cur),
        count_(count)
    {
    }

    inline const T& operator*(void)const;
    inline const T* operator->(void)const;
    inline FixedFifoIterator& operator++(void);
    inline FixedFifoIterator operator++(int);
    inline bool operator==(const FixedFifoIterator& rhs) const;
    inline bool operator!=(const FixedFifoIterator& rhs) const;

    inline operator const_iterator(void) const
    {
        return const_iterator(start_, end_, current_, count_);
    }

private:
    T* start_;
    T* end_;
    T* current_;
    size_t count_;
};

template<typename T, size_t N>
struct FixedFifoConstIterator
{
    FixedFifoConstIterator(T* start, T* end, T* cur, size_t count) :
        start_(start),
        end_(end),
        current_(cur),
        count_(count)
    {
    }

    inline const T& operator*(void)const;
    inline const T* operator->(void)const;
    inline FixedFifoConstIterator& operator++(void);
    inline FixedFifoConstIterator operator++(int);
    inline bool operator==(const FixedFifoConstIterator& rhs) const;
    inline bool operator!=(const FixedFifoConstIterator& rhs) const;

private:
    T* start_;
    T* end_;
    T* current_;
    size_t count_;
};

template<typename T, size_t N>
class FixedFifo
{
public:
    typedef T Type;
    typedef T value_type;
    typedef size_t size_type;
    typedef FixedFifoIterator<T, N> iterator;
    typedef FixedFifoConstIterator<T, N> const_iterator;
    typedef T& Reference;
    typedef T& reference;
    typedef const T& ConstReference;
    typedef const T& const_reference;

public:
    X_INLINE FixedFifo(void);
    X_INLINE ~FixedFifo(void);

    X_INLINE T& operator[](size_type idx);
    X_INLINE const T& operator[](size_type idx) const;

    // push a item on to the internal ring buffer
    X_INLINE void push(const T& v);
    X_INLINE void push(T&& v);

    template<class... ArgsT>
    X_INLINE void emplace(ArgsT&&... args);

    // pop a item from the buffer
    X_INLINE void pop(void);

    // returns the topmost value
    X_INLINE T& peek(void);
    // returns the topmost value
    X_INLINE const T& peek(void) const;

    // items are cleared but buffer is not returned.
    X_INLINE void clear(void);
    // items are cleared and buffer is destroyed
    X_INLINE void free(void);

    // returns the number of items currently inside the fifo
    X_INLINE size_type size(void) const;
    // returns the total capacity.
    X_INLINE size_type capacity(void) const;
    X_INLINE size_type freeSpace(void) const;

    X_INLINE bool isEmpty(void) const;
    X_INLINE bool isNotEmpty(void) const;

    // STL iterators.
    iterator begin(void);
    iterator end(void);

    const_iterator begin(void) const;
    const_iterator end(void) const;

    Reference front(void);
    ConstReference front(void) const;
    Reference back(void);
    ConstReference back(void) const;

private:
    X_INLINE T* startPtr(void);
    X_INLINE const T* startPtr(void) const;

    X_INLINE T* endPtr(void);
    X_INLINE const T* endPtr(void) const;

private:
    X_NO_COPY(FixedFifo);
    X_NO_ASSIGN(FixedFifo);

    // T  array_[N];
    uint8_t X_ALIGNED_SYMBOL(array_[N * sizeof(T)], X_ALIGN_OF(T));

    T* read_;
    T* write_;

    size_type num_;
};

#include "FixedFifo.inl"

X_NAMESPACE_END

#endif // !_X_CON_FIXED_FIFO_H_
