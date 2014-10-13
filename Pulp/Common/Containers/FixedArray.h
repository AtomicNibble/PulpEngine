#pragma once

#ifndef X_CON_FIXEDARRAY_H
#define X_CON_FIXEDARRAY_H


X_NAMESPACE_BEGIN(core)

template <typename T, size_t N>
class FixedArray
{
public:
	typedef T  Type;
	typedef T* TypePtr;
	typedef const T* ConstTypePtr;
	typedef T* iterator;
	typedef const T* const_iterator;
	typedef size_t size_type;

	FixedArray();
	FixedArray(const T& initalval);
	~FixedArray(void);

	// index operators
	const T& operator[](size_type idx) const;
	T& operator[](size_type idx);

	// returns a pointer to the list
	T* ptr(void);
	const T* ptr(void) const;

	// clear the list, no memory free
	inline void clear(void);

	// append element (same as push_back)
	inline size_type append(const T& obj);
	// appends a item to the end, resizing if required.
	inline size_type push_back(const T& obj);

	inline iterator insert(iterator position, const T& val);

	// any iterms in the array
	inline bool isEmpty(void) const;

	inline size_type size(void) const;
	// returns number of elements allocated for
	inline size_type capacity(void) const;

	// for STL use
	inline iterator begin(void);
	inline const_iterator begin(void) const;
	inline iterator end(void);
	inline const_iterator end(void) const;


private:
	size_type size_;
	T	array_[N];
};

#include "FixedArray.inl"

X_NAMESPACE_END

#endif // X_CON_FIXEDARRAY_H
