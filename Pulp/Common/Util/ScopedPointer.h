#pragma once


#ifndef X_UTIL_SCOPED_POINTER_H_
#define X_UTIL_SCOPED_POINTER_H_

#include <CompileTime\IsPointer.h>

X_NAMESPACE_BEGIN(core)


template<typename T>
class ScopedPointer
{
	ScopedPointer();
public:
	typedef typename T* pointer;

	explicit ScopedPointer(core::MemoryArenaBase* arena, nullptr_t) :
		pInstance_(pointer()),
		arena_(arena)
	{
		X_ASSERT_NOT_NULL(arena);
	}

	ScopedPointer(core::MemoryArenaBase* arena, T* pInstance) :
		pInstance_(pInstance),
		arena_(arena)
	{
		X_ASSERT_NOT_NULL(pInstance);
		X_ASSERT_NOT_NULL(arena);
	}


	ScopedPointer(ScopedPointer&& oth) :
		pInstance_(oth.release()),
		arena_(oth.arena_)
	{
	}

	~ScopedPointer() {	
		if (pInstance_ != pointer()) {
			deleter(pInstance_);
		}
	}

	ScopedPointer& operator=(nullptr_t)
	{	
		// assign a null pointer
		reset();
		return *this;
	}

	ScopedPointer& operator=(ScopedPointer&& rhs) 
	{	
		// assign by moving _Right
		if (this != &rhs)
		{	
			// different, do the move
			reset(rhs.release());
			arena_ = oth.arena_;
		}
		return *this;
	}


	X_INLINE T& operator* () const
	{
		return *pInstance_;
	}

	X_INLINE pointer operator-> () const
	{
		return pInstance_;
	}

	X_INLINE pointer get(void) const {
		return pInstance_;
	}

	X_INLINE core::MemoryArenaBase* getArena(void) const {
		return arena_;
	}

	pointer release(void)
	{	
		// yield ownership of pointer
		pointer pAns = get();
		pInstance_ = pointer();
		return pAns;
	}

	X_INLINE void reset(pointer ptr = pointer())
	{	
		// establish new pointer
		pointer pOld = get();
		pInstance_ = ptr;
		if (pOld != pointer()) {
			deleter(pOld);
		}
	}

	X_NO_ASSIGN(ScopedPointer);
	X_NO_COPY(ScopedPointer);

private:
	void deleter(pointer ptr) {
		X_DELETE(ptr, arena_);
	}

private:
	T* pInstance_;
	core::MemoryArenaBase* arena_;
};


template<typename T>
class ScopedPointer<T[]>
{
	ScopedPointer();
public:
	typedef typename T* pointer;

	explicit ScopedPointer(core::MemoryArenaBase* arena, nullptr_t) :
		pInstance_(pointer()),
		arena_(arena)
	{
		X_ASSERT_NOT_NULL(arena);
	}


	ScopedPointer(core::MemoryArenaBase* arena, T* pInstance) :
		pInstance_(pInstance),
		arena_(arena)
	{
		X_ASSERT_NOT_NULL(pInstance);
		X_ASSERT_NOT_NULL(arena);
	}

	ScopedPointer(ScopedPointer&& oth) :
		pInstance_(oth.release()),
		arena_(oth.arena_)
	{
	}

	~ScopedPointer() {
		if (pInstance_) {
			deleter(pInstance_);
		}
 	}

	ScopedPointer& operator=(nullptr_t)
	{
		// assign a null pointer
		reset();
		return *this;
	}

	ScopedPointer& operator=(ScopedPointer&& rhs)
	{
		// assign by moving _Right
		if (this != &rhs)
		{
			// different, do the move
			reset(rhs.release());
			arena_ = oth.arena_;
		}
		return *this;
	}

	X_INLINE T& operator* () const
	{
		return *pInstance_;
	}

	X_INLINE T* operator-> () const
	{
		return pInstance_;
	}

	X_INLINE T* get(void) const
	{
		return pInstance_;
	}

	X_INLINE core::MemoryArenaBase* getArena(void) const {
		return arena_;
	}

	pointer release(void)
	{
		// yield ownership of pointer
		pointer pAns = get();
		pInstance_ = pointer();
		return pAns;
	}

	X_INLINE void reset(pointer ptr = pointer())
	{
		// establish new pointer
		pointer pOld = get();
		pInstance_ = ptr;
		if (pOld != pointer()) {
			deleter(pOld);
		}
	}


	X_NO_ASSIGN(ScopedPointer);
	X_NO_COPY(ScopedPointer);

private:
	void deleter(pointer ptr) {
		X_DELETE_ARRAY(ptr, arena_);
	}


private:
	T* pInstance_;
	core::MemoryArenaBase* arena_;
};

X_NAMESPACE_END

#endif // !X_UTIL_SCOPED_POINTER_H_