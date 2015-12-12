#pragma once

#ifndef X_CON_ARRAY_H
#define X_CON_ARRAY_H

#include <ISerialize.h>

#include "CompileTime\IsPOD.h"

X_NAMESPACE_BEGIN(core)

template <typename T>
class Array
{
public:
	typedef T  Type;
	typedef T* TypePtr;
	typedef const T* ConstTypePtr;
	typedef T* Iterator;
	typedef const T* ConstIterator;
	typedef size_t size_type;
	typedef T& Reference;
	typedef const T& ConstReference;

	enum : size_type {
		invalid_index = static_cast<size_type>(-1)
	};

	Array(MemoryArenaBase* arena);
	Array(MemoryArenaBase* arena, size_type size);
	Array(MemoryArenaBase* arena, size_type size, const T& initialValue);
	Array(const Array& oth);
	Array(Array&& oth);
	virtual ~Array(void);

	void setArena(MemoryArenaBase* arena);
	void setArena(MemoryArenaBase* arena, size_type capacity);

	Array<T>& operator=(const Array<T>& oth);
	Array<T>& operator=(Array<T>&& oth);

	// index operators
	const T& operator[](size_type idx) const;
	T& operator[](size_type idx);

	// returns a pointer to the list
	T* ptr(void);									
	const T* ptr(void) const;

	X_INLINE const bool isEmpty(void) const;
	X_INLINE const bool isNotEmpty(void) const;

	// clear the list, no memory free
	void clear(void);		
	// clear the list, and Free memory
	void free(void);
	// returns number of elements in list
	size_type size(void) const;
	// returns number of elements allocated for
	size_type capacity(void) const;
	// set new granularity
	void setGranularity(size_type newgranularity);
	// get the current granularity
	size_type granularity(void) const;


	// Inserts or erases elements at the end such that size is 'size'
	void resize(size_type size, const T& t = T());
	// increases the capacity to the given amount, only if it's greater than current.
	// dose not increase the amount of items.
	void reserve(size_type size);


	Type& AddOne(void);

	// append element (same as push_back)
	size_type append(const T& obj);
	size_type append(T&& obj);
	// add the list
	size_type append(const Array<T>& oth);
	// appends a item to the end, resizing if required.
	size_type push_back(const T& obj);
	size_type push_back(T&& obj);

	// Removes the last element, it is deconstructed
	void pop_back(void);


	// insert the element at the given index
	size_type insert(const Type& obj, size_type index = 0);
	size_type insert(Type&& obj, size_type index = 0);
	// remove the element at the given index
	bool removeIndex(size_type idx);

	// returns invalid_index when not found.
	size_type find(const Type& val) const;

	// swaps the arrays
	// A = B & B = A
	void swap(Array& oth);

	// for STL use
	inline Iterator begin(void);
	inline ConstIterator begin(void) const;
	inline Iterator end(void);
	inline ConstIterator end(void) const;
	inline Reference back(void);
	inline ConstReference back(void) const;

	// ISerialize
	virtual bool SSave(XFile* pFile) const X_FINAL;
	virtual bool SLoad(XFile* pFile) X_FINAL;
	// ~ISerialize

private:
	// used to make sure buffer is large enougth.
	// if it grows exsisting values are copyied into new buffer.
	void ensureSize(size_type size);

	// for easy allocation custimisation.
	X_INLINE void DeleteArr(T* pArr);
	X_INLINE T* Allocate(size_type size);

	T*				list_;			// pointer to memory block
	size_type		num_;			// num elemets stored in the list
	size_type		size_;			// the current allocated size
	size_type		granularity_;	// the allocation size stratergy

	MemoryArenaBase* arena_;
};

#include "Array.inl"

X_NAMESPACE_END


#endif // X_CON_ARRAY_H
