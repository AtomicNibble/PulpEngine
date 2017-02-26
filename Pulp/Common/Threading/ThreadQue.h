#pragma once


#ifndef X_THREADING_QUE_H_
#define X_THREADING_QUE_H_

#include "Containers\Fifo.h"
#include "Threading\Signal.h"
#include "Threading\ConditionVariable.h"
#include "Threading\Spinlock.h"

X_NAMESPACE_BEGIN(core)

// A simple thread que.


template<typename T, typename SynchronizationPrimitive>
class ThreadQue
{
public:
	typedef size_t size_type;

public:
	ThreadQue(core::MemoryArenaBase* arena);
	ThreadQue(core::MemoryArenaBase* arena, size_type num);
	~ThreadQue();

	void reserve(size_type num);
	void clear(void);

	void push(T const& value);
	// pushes if predicate don't match.
	// true if pushed.
	template<class UnaryPredicate>
	bool push_unique_if(T const& value, UnaryPredicate p);

	bool tryPop(T& value);

	size_type size(void) const;
	bool isEmpty(void) const;
	bool isNotEmpty(void) const;

protected:
	typedef core::Fifo<T> Que;

	Que que_;
	SynchronizationPrimitive primitive_;
};



template<typename T, typename SynchronizationPrimitive>
class ThreadQueBlocking : public ThreadQue<T, SynchronizationPrimitive>
{
public:
	using ThreadQue<T, SynchronizationPrimitive>::ThreadQue;

	void push(T const& value);
	bool tryPop(T& value);
	void pop(T& value);
	T pop(void);

private:
	core::Signal signal_;
};


template<typename T>
class ThreadQueBlocking<T, core::CriticalSection> : public ThreadQue<T, core::CriticalSection>
{
public:
	using ThreadQue<T, CriticalSection>::ThreadQue;

	void push(T const& value);
	void pop(T& value);
	T pop(void);

private:
	core::ConditionVariable cond_;
};

X_NAMESPACE_END

#include "ThreadQue.inl"

#endif // !X_THREADING_QUE_H_
