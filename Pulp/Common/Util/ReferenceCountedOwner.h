#pragma once
#ifndef X_REFERENCECOUNTEDOWNER_H
#define X_REFERENCECOUNTEDOWNER_H

X_NAMESPACE_BEGIN(core)

class MemoryArenaBase;

template<class T>
class ReferenceCountedOwner
{
public:
    // Gains ownership of the given instance, deleting it using the provided arena as soon as its reference count reaches zero.
    X_INLINE ReferenceCountedOwner(T* instance, MemoryArenaBase* arena);
    X_INLINE ReferenceCountedOwner(const ReferenceCountedOwner<T>& other);
    X_INLINE ReferenceCountedOwner(ReferenceCountedOwner<T>&& other);
    X_INLINE ~ReferenceCountedOwner(void);

    X_INLINE ReferenceCountedOwner<T>& operator=(const ReferenceCountedOwner<T>& other);
    X_INLINE ReferenceCountedOwner<T>& operator=(ReferenceCountedOwner<T>&& other);

    X_INLINE T* operator->(void);
    X_INLINE const T* operator->(void)const;

    X_INLINE T* instance(void);
    X_INLINE const T* instance(void) const;

private:
    T* instance_;
    MemoryArenaBase* arena_;
};

#include "ReferenceCountedOwner.inl"

X_NAMESPACE_END

#endif
