#pragma once
#ifndef X_REFERENCECOUNTED_H
#define X_REFERENCECOUNTED_H


X_NAMESPACE_BEGIN(core)

// ReferenceCountedInstance can be used to keep one copy of a instance so that the memory address
// does not change if when the owner does.
template <class T>
class ReferenceCountedInstance
{
public:
	X_INLINE ReferenceCountedInstance(void);
	X_INLINE explicit ReferenceCountedInstance(const T& instance);

	X_INLINE uint32_t addReference(void);
	X_INLINE uint32_t removeReference(void);

	// returns the ref count, used in the UT mainly.
	X_INLINE uint32_t getRefCount(void) const;


	X_INLINE T* instance(void);
	X_INLINE const T* instance(void) const;
private:
	T instance_;
	mutable uint32_t refCount_;
};


// this can be used to inhert a instance
// to give it refrence counting 
template <class T>
class ReferenceCounted
{
public:
	X_INLINE ReferenceCounted(void);

	X_INLINE uint32_t addReference(void);
	X_INLINE uint32_t removeReference(void);
	X_INLINE uint32_t getRefCount(void) const;

private:
	mutable uint32_t refCount_;
};


#include "ReferenceCounted.inl"

X_NAMESPACE_END


#endif
