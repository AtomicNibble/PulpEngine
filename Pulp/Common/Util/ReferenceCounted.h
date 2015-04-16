#pragma once
#ifndef X_REFERENCECOUNTED_H
#define X_REFERENCECOUNTED_H


X_NAMESPACE_BEGIN(core)

//
// a refrence counter object for use with arena
//
// I should really use this for everything no?
//
//
//
template <class T>
class ReferenceCountedArena
{
public:
	X_INLINE ReferenceCountedArena(void);
	X_INLINE explicit ReferenceCountedArena(const T& instance);

	X_INLINE uint32_t addReference(void) const;
	X_INLINE uint32_t removeReference(void) const;

	// returns the ref count, used in the UT mainly.
	X_INLINE uint32_t getRefCount(void) const;

private:
	mutable uint32_t refCount_;
};


template<typename T>
class ReferenceCounted
{
public:
	X_INLINE ReferenceCounted();
	X_INLINE virtual ~ReferenceCounted();

	X_INLINE void addRef(void);
	X_INLINE uint32_t release(void);

private:
	mutable uint32_t refCount_;
};


#include "ReferenceCounted.inl"

X_NAMESPACE_END


#endif
