#pragma once

#ifndef X_THREADING_QUE_H_
#define X_THREADING_QUE_H_

#include "Containers\FixedFifo.h"
#include "Threading\Signal.h"
#include "Threading\ConditionVariable.h"
#include "Threading\Spinlock.h"

X_NAMESPACE_BEGIN(core)

// A fixed sized thread Que, it's blocking by default.
// there is little point to a none blocking one, even if you check if space first.
// since you don't own the lock between checking size and pushing, so you could loose = crash
// the only time that would work is for single producer

template<typename T, size_t N, typename SynchronizationPrimitive>
class FixedThreadQueBase
{
public:
    typedef size_t size_type;
    typedef core::FixedFifo<T, N> FixedQue;

public:
    void clear(void);
    void free(void);

    // only safe to make a copy.
    typename std::enable_if<std::is_trivial<T>::value, T>::type
    peek(void);

    size_type size(void);
    size_type freeSpace(void);
    size_type capacity(void) const;
    bool isEmpty(void) const;
    bool isNotEmpty(void) const;

protected:
    FixedQue que_;
    SynchronizationPrimitive primitive_;
};

template<typename T, size_t N, typename SynchronizationPrimitive>
class FixedThreadQue : public FixedThreadQueBase<T, N, SynchronizationPrimitive>
{
public:
    FixedThreadQue() = default;

    void push(T const& value);
    void push(T&& value);
    bool tryPop(T& value);
    void pop(T& value);
    T pop(void);

private:
    core::Signal signal_;
};

template<typename T, size_t N>
class FixedThreadQue<T, N, core::CriticalSection> : public FixedThreadQueBase<T, N, core::CriticalSection>
{
public:
    FixedThreadQue() = default;

    void push(T const& value);
    void push(T&& value);
    bool tryPop(T& value);
    void pop(T& value);
    T pop(void);

private:
    core::ConditionVariable cond_;
    core::ConditionVariable postPopCond_;
};

X_NAMESPACE_END

#include "FixedThreadQue.inl"

#endif // !X_THREADING_QUE_H_
