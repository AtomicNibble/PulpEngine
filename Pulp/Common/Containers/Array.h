#pragma once

#ifndef X_CON_ARRAY_H
#define X_CON_ARRAY_H

#include <ISerialize.h>

#include "CompileTime\IsPOD.h"
#include "Containers\ArrayAllocators.h"

X_NAMESPACE_BEGIN(core)

class MemoryArenaBase;
struct XFile;

template <typename T, class Allocator = ArrayAllocator<T>>
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
	Array(MemoryArenaBase* arena, std::initializer_list<T> iList);
	Array(const Array& oth);
	Array(Array&& oth);
	~Array(void);

	void setArena(MemoryArenaBase* arena);
	void setArena(MemoryArenaBase* arena, size_type capacity);
	core::MemoryArenaBase* getArena(void) const; // have one use case for this currently lol.

	Allocator& getAllocator(void);
	const Allocator& getAllocator(void) const;

	Array<T, Allocator>& operator=(std::initializer_list<T> iList);
	Array<T, Allocator>& operator=(const Array<T, Allocator>& oth);
	Array<T, Allocator>& operator=(Array<T, Allocator>&& oth);

	// index operators
	const T& operator[](size_type idx) const;
	T& operator[](size_type idx);

	// returns a pointer to the list
	T* ptr(void);									
	const T* ptr(void) const;
	T* data(void);
	const T* data(void) const;


	X_INLINE const bool isEmpty(void) const;
	X_INLINE const bool isNotEmpty(void) const;

	// clear the list, no memory free
	void clear(void);		
	// clear the list, and Free memory
	void free(void);
	// attempts to make capacity == size, not guaranteed.
	void shrinkToFit(void);
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

	template<class... Args>
	Type& AddOne(Args&&... args);

	// append element (same as push_back)
	size_type append(const T& obj);
	size_type append(T&& obj);
	// add the list
	size_type append(const Array<T, Allocator>& oth);
	// appends a item to the end, resizing if required.
	size_type push_back(const T& obj);
	size_type push_back(T&& obj);

	template<class... ArgsT>
	size_type emplace_back(ArgsT&&... obj);

	// Removes the last element, it is deconstructed
	void pop_back(void);


	// insert the element at the given index
	size_type insert(const Type& obj, size_type index = 0);
	size_type insert(Type&& obj, size_type index = 0);
	// remove the element at the given index
	bool removeIndex(size_type idx);
	void remove(const T& item);

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
	inline Reference front(void);
	inline ConstReference front(void) const;
	inline Reference back(void);
	inline ConstReference back(void) const;

	// ISerialize
	bool SSave(XFile* pFile) const;
	bool SLoad(XFile* pFile);
	// ~ISerialize

protected:
	// used to make sure buffer is large enougth.
	// if it grows exsisting values are copyied into new buffer.
	void ensureSize(size_type size);

	// for easy allocation custimisation.
	X_INLINE void DeleteArr(T* pArr);
	X_INLINE T* Allocate(size_type size);

protected:
	T*				list_;			// pointer to memory block
	size_type		num_;			// num elemets stored in the list
	size_type		size_;			// the current allocated size
	size_type		granularity_;	// the allocation size stratergy

	Allocator		allocator_;
};

#include "Array.inl"

X_NAMESPACE_END


#endif // X_CON_ARRAY_H
