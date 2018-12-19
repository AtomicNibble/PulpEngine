#pragma once

#ifndef X_THREADING_QUE_H_
#define X_THREADING_QUE_H_

#include "Containers\Fifo.h"
#include "Threading\Signal.h"
#include "Threading\ConditionVariable.h"
#include "Threading\Spinlock.h"

X_NAMESPACE_BEGIN(core)

// A simple thread que.

template<typename QueT, typename SynchronizationPrimitive>
class ThreadQueBase
{
public:
    typedef size_t size_type;
    typedef typename QueT::Type Type;

public:
    ThreadQueBase(core::MemoryArenaBase* arena);
    ThreadQueBase(core::MemoryArenaBase* arena, size_type num);
    ~ThreadQueBase();

    void reserve(size_type num);
    void clear(void);
    void free(void);

    void push(Type const& value);
    void push(Type&& value);
    // pushes if predicate don't match.
    // true if pushed.
    template<class UnaryPredicate>
    bool push_unique_if(Type const& value, UnaryPredicate p);

    bool tryPop(Type& value);
    template<class CallBack>
    void popAll(CallBack func);

    size_type size(void);
    bool isEmpty(void) const;
    bool isNotEmpty(void) const;

protected:
    QueT que_;
    SynchronizationPrimitive primitive_;
};


template<typename QueT, typename SynchronizationPrimitive>
class ThreadQueBlockingBase : public ThreadQueBase<QueT, SynchronizationPrimitive>
{
    typedef ThreadQueBase<QueT, SynchronizationPrimitive> BaseT;
    typedef typename BaseT::Type Type;

public:
    using BaseT::BaseT;

    void push(Type const& value);
    void push(Type&& value);
    void pop(Type& value);
    Type pop(void);

private:
    core::Signal signal_;
};

template<typename QueT>
class ThreadQueBlockingBase<QueT, core::CriticalSection> : public ThreadQueBase<QueT, core::CriticalSection>
{
    typedef ThreadQueBase<QueT, core::CriticalSection> BaseT;
    typedef typename BaseT::Type Type;

public:
    using BaseT::BaseT;

    void push(Type const& value);
    void push(Type&& value);
    void pop(Type& value);
    Type pop(void);

private:
    core::ConditionVariable cond_;
};

template<typename T, typename SynchronizationPrimitive>
using ThreadQue = ThreadQueBase<core::Fifo<T>, SynchronizationPrimitive>;

template<typename T, typename SynchronizationPrimitive>
using ThreadQueBlocking = ThreadQueBlockingBase<core::Fifo<T>, SynchronizationPrimitive>;

X_NAMESPACE_END

#include "ThreadQue.inl"

#endif // !X_THREADING_QUE_H_
