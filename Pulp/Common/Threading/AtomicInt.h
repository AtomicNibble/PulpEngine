#pragma once

#ifndef _X_THREADING_ATOMIC_INT_H_
#define _X_THREADING_ATOMIC_INT_H_

#include "Threading\Atomic.h"

X_NAMESPACE_BEGIN(core)

X_DISABLE_WARNING(4522) // multiple assignment operators specified

class AtomicInt
{
public:
    /// Constructs an AtomicInt from an integer.
    X_INLINE AtomicInt();
    X_INLINE explicit AtomicInt(int32_t value);

    X_INLINE int32_t operator=(int32_t value) volatile;
    X_INLINE int32_t operator=(int32_t value);
    X_INLINE int32_t operator++(void) volatile;
    X_INLINE int32_t operator++(void);
    X_INLINE int32_t operator--(void) volatile;
    X_INLINE int32_t operator--(void);

    X_INLINE int32_t operator+=(int32_t value) volatile;
    X_INLINE int32_t operator+=(int32_t value);
    X_INLINE int32_t operator-=(int32_t value) volatile;
    X_INLINE int32_t operator-=(int32_t value);
    X_INLINE int32_t operator&=(int32_t value) volatile;
    X_INLINE int32_t operator&=(int32_t value);
    X_INLINE int32_t operator|=(int32_t value) volatile;
    X_INLINE int32_t operator|=(int32_t value);

    X_INLINE operator int32_t(void) const volatile;
    X_INLINE operator int32_t(void) const;

private:
    X_NO_COPY(AtomicInt);
    X_NO_ASSIGN(AtomicInt);
    X_NO_ASSIGN_VOLATILE(AtomicInt);

    X_ALIGNED_SYMBOL(int32_t value_, 4);
};

#include "AtomicInt.inl"

X_ENABLE_WARNING(4522)

X_NAMESPACE_END

#endif // !_X_THREADING_ATOMIC_INT_H_
