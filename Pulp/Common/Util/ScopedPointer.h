#pragma once


#ifndef X_UTIL_SCOPED_POINTER_H_
#define X_UTIL_SCOPED_POINTER_H_

#include <CompileTime\IsPointer.h>

X_NAMESPACE_BEGIN(core)


template<typename T>
class ScopedPointer
{
public:
	ScopedPointer(T* pInstance, core::MemoryArenaBase* arena) :
		pInstance_(pInstance),
		arena_(arena)
	{
		X_ASSERT_NOT_NULL(pInstance);
		X_ASSERT_NOT_NULL(arena);
	}

	~ScopedPointer() {
		X_DELETE(pInstance_, arena_);
	}

private:
	T* pInstance_;
	core::MemoryArenaBase* arena_;
};


template<typename T>
class ScopedPointer<T[]>
{
public:
	ScopedPointer(T* pInstance, core::MemoryArenaBase* arena) :
		pInstance_(pInstance),
		arena_(arena)
	{
		X_ASSERT_NOT_NULL(pInstance);
		X_ASSERT_NOT_NULL(arena);
	}

	~ScopedPointer() {
		X_DELETE_ARRAY(pInstance_, arena_);
	}

private:
	T* pInstance_;
	core::MemoryArenaBase* arena_;
};

X_NAMESPACE_END

#endif // !X_UTIL_SCOPED_POINTER_H_