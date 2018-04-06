#pragma once

#ifndef X_POINTER_64_H_
#define X_POINTER_64_H_

X_NAMESPACE_BEGIN(core)

template<typename T>
class Pointer64
{
public:
    Pointer64();

    X_INLINE Pointer64& operator=(size_t val);
    X_INLINE Pointer64& operator=(T* p);
    X_INLINE operator T*() const;

    X_INLINE Pointer64& operator+=(const Pointer64& oth);

    template<typename Type>
    X_INLINE Type* as(void) const;

    X_INLINE void* asVoid(void) const;

    X_INLINE const T* operator[](size_t i) const;
    X_INLINE T* operator[](size_t i);

private:
    //should be ok since same type.
    //	X_NO_ASSIGN(Pointer64);

    uint64_t raw_;
};

#include "Pointer64.inl"

X_NAMESPACE_END

#endif // !X_POINTER_64_H_