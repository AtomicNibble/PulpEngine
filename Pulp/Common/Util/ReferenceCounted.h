#pragma once
#ifndef X_REFERENCECOUNTED_H
#define X_REFERENCECOUNTED_H

X_NAMESPACE_BEGIN(core)

// ReferenceCountedInstance can be used to keep one copy of a instance so that the memory address
// does not change if when the owner does.
template<class T, typename Primative = int32_t>
class ReferenceCountedInstance
{
public:
    X_INLINE ReferenceCountedInstance(void);
    X_INLINE explicit ReferenceCountedInstance(const T& instance);

    // these are const as the class data don't change
    X_INLINE int32_t addReference(void) const;
    X_INLINE int32_t removeReference(void) const;
    X_INLINE int32_t getRefCount(void) const;

    X_INLINE T* instance(void);
    X_INLINE const T* instance(void) const;

private:
    T instance_;
    mutable Primative refCount_;
};

template<class T, typename Primative = int32_t>
class ReferenceCountedInherit : public T
{
public:
    typedef Primative PrimativeType;
    using T::T;

    // these are const as the class data don't change
    X_INLINE int32_t addReference(void) const;
    X_INLINE int32_t removeReference(void) const;
    X_INLINE int32_t getRefCount(void) const;

    X_INLINE T* instance(void);
    X_INLINE const T* instance(void) const;

private:
    mutable Primative refCount_{1};
};

// inherit this to add ref counting.
template<typename Primative = int32_t>
class ReferenceCounted
{
public:
    X_INLINE ReferenceCounted(void);

    // these are const as the class data don't change
    X_INLINE int32_t addReference(void) const;
    X_INLINE int32_t removeReference(void) const;
    X_INLINE int32_t getRefCount(void) const;

private:
    mutable Primative refCount_;
};

#include "ReferenceCounted.inl"

X_NAMESPACE_END

#endif
