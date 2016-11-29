#pragma once


#ifndef X_UTIL_SCOPED_POINTER_H_
#define X_UTIL_SCOPED_POINTER_H_

#include <CompileTime\IsPointer.h>

X_NAMESPACE_BEGIN(core)


template<typename T>
class UniquePointerBase
{
public:
	typedef typename T* pointer;

	X_INLINE UniquePointerBase(core::MemoryArenaBase* arena, T* pInstance);

	X_INLINE core::MemoryArenaBase* getArena(void) const;
	X_INLINE pointer& ptr(void);
	X_INLINE const pointer& ptr(void) const;

private:
	pointer pInstance_;
protected:
	core::MemoryArenaBase* arena_;
};


template<typename T>
class UniquePointer : public UniquePointerBase<T>
{
	UniquePointer();
public:
	typedef UniquePointerBase<T> Mybase;
	typedef typename Mybase::pointer pointer;

	X_INLINE explicit UniquePointer(core::MemoryArenaBase* arena);
	X_INLINE explicit UniquePointer(core::MemoryArenaBase* arena, nullptr_t);

	X_INLINE UniquePointer(core::MemoryArenaBase* arena, pointer pInstance);

	X_INLINE UniquePointer(UniquePointer&& oth);
	X_INLINE ~UniquePointer();

	X_INLINE UniquePointer& operator=(nullptr_t);
	X_INLINE UniquePointer& operator=(UniquePointer&& rhs);

	X_INLINE T& operator* () const;
	X_INLINE pointer operator-> () const;
	X_INLINE pointer get(void) const;
	X_INLINE explicit operator bool() const;


	X_INLINE pointer release(void);
	X_INLINE void reset(pointer ptr = pointer());

	X_INLINE void swap(UniquePointer& oth);

private:
	void deleter(pointer ptr) {
		X_DELETE(ptr, getArena());
	}
};

template<typename T>
class UniquePointer<T[]> : public UniquePointerBase<T>
{
	UniquePointer();
public:
	typedef UniquePointerBase<T> Mybase;
	typedef typename Mybase::pointer pointer;

	X_INLINE explicit UniquePointer(core::MemoryArenaBase* arena);
	X_INLINE explicit UniquePointer(core::MemoryArenaBase* arena, nullptr_t);

	X_INLINE UniquePointer(core::MemoryArenaBase* arena, pointer pInstance);

	X_INLINE UniquePointer(UniquePointer&& oth);
	X_INLINE ~UniquePointer();

	X_INLINE UniquePointer& operator=(nullptr_t);
	X_INLINE UniquePointer& operator=(UniquePointer&& rhs);

	X_INLINE T& operator* () const;
	X_INLINE pointer operator-> () const;
	X_INLINE pointer get(void) const;
	X_INLINE explicit operator bool() const;


	X_INLINE pointer release(void);
	X_INLINE void reset(pointer ptr = pointer());

	X_INLINE void swap(UniquePointer& oth);

private:
	void deleter(pointer ptr) {
		X_DELETE_ARRAY(ptr, getArena());
	}
};


template<typename T, class... _Types>
X_INLINE core::UniquePointer<T> makeUnique(core::MemoryArenaBase* arena, _Types&&... _Args)
{
	return core::UniquePointer<T>(arena, X_NEW(T, arena, "makeUnique<T>")(std::forward<_Types>(_Args)...));
}


X_NAMESPACE_END

#include "UniquePointer.inl"

#endif // !X_UTIL_SCOPED_POINTER_H_