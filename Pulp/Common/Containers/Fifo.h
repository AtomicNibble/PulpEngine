#pragma once


#ifndef _X_CON_FIFIO_H_
#define _X_CON_FIFIO_H_

X_NAMESPACE_BEGIN(core)


template<typename T>
class Fifo;

template<typename T>
struct _FifoIterator;

template<typename T>
struct _FifoConstIterator;


template<typename T>
struct _FifoIterator
{
	typedef _FifoConstIterator<T> const_iterator;

	_FifoIterator(T* start, T* end, T* cur, size_t count) 
		: start_(start), end_(end), current_(cur), count_(count) {}

	inline const T& operator*(void) const;
	inline const T* operator->(void) const;
	inline _FifoIterator& operator++(void);
	inline _FifoIterator operator++(int);
	inline bool operator==(const _FifoIterator& rhs) const;
	inline bool operator!=(const _FifoIterator& rhs) const;

	inline operator const_iterator(void) const {
		return const_iterator(start_, end_, current_, count_);
	}
private:
	T* start_;
	T* end_;
	T* current_;
	size_t count_;
};

template<typename T>
struct _FifoConstIterator
{
	_FifoConstIterator(T* start, T* end, T* cur, size_t count) 
	: start_(start), end_(end), current_(cur), count_(count) {}

	inline const T& operator*(void) const;
	inline const T* operator->(void) const;
	inline _FifoConstIterator& operator++(void);
	inline _FifoConstIterator operator++(int);
	inline bool operator==(const _FifoConstIterator& rhs) const;
	inline bool operator!=(const _FifoConstIterator& rhs) const;

private:
	T* start_;
	T* end_;
	T* current_;
	size_t count_;
};




template<typename T>
class Fifo
{
public:
	typedef size_t size_type;
	typedef _FifoIterator<T> iterator;
	typedef _FifoConstIterator<T> const_iterator;
	typedef T& Reference;
	typedef T& reference;
	typedef const T& ConstReference;
	typedef const T& const_reference;

	X_INLINE Fifo(MemoryArenaBase* arena);
	X_INLINE Fifo(MemoryArenaBase* arena, size_type size);
	X_INLINE Fifo(const Fifo& oth);
	X_INLINE Fifo(Fifo&& oth);
	X_INLINE ~Fifo(void);

	X_INLINE Fifo& operator=(const Fifo& oth);
	X_INLINE Fifo& operator=(Fifo&& oth);

	X_INLINE T& operator[](size_type idx);
	X_INLINE const T& operator[](size_type idx) const;

	X_INLINE void setArena(MemoryArenaBase* arena);

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

	X_INLINE bool contains(const T& oth) const;

	template<class UnaryPredicate>
	X_INLINE bool contains_if(UnaryPredicate p) const;

	// sets the size of the buffer
	X_INLINE void reserve(size_type num);

	// items are cleared but buffer is not returned.
	X_INLINE void clear(void);
	// items are cleared and buffer is destroyed
	X_INLINE void free(void);
    // attempts to make capacity == size, not guaranteed.
    X_INLINE void shrinkToFit(void);

	// returns the number of items currently inside the fifo
	X_INLINE size_type size(void) const;
	// returns the total capacity.
	X_INLINE size_type capacity(void) const;

	// any items?
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
	void expand(void);

	// for easy memory allocation changes later.
	X_INLINE void Delete(T* pData);
	X_INLINE T* Allocate(size_type num);

private:
	T* start_;
	T* end_;
	T* read_;
	T* write_;

	MemoryArenaBase* arena_;
};

#include "Fifo.inl"

X_NAMESPACE_END


#endif // !_X_CON_FIFIO_H_
