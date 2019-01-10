#pragma once

#ifndef X_POINTER_64_H_
#define X_POINTER_64_H_

X_NAMESPACE_BEGIN(core)

template<typename T>
class Pointer64
{
public:
    using type = T;
    using pointer = type *;
    using const_pointer = const type*;

public:
    Pointer64();

    X_INLINE Pointer64& operator=(size_t val);
    X_INLINE Pointer64& operator=(T* p);

    X_INLINE Pointer64& operator+=(const Pointer64& oth);

    X_INLINE operator pointer() const;
    X_INLINE pointer operator->() const;

    template<typename Type>
    X_INLINE Type* as(void) const;

    X_INLINE void* asVoid(void) const;

    X_INLINE const_pointer operator[](size_t i) const;
    X_INLINE pointer operator[](size_t i);

private:
    uint64_t raw_;
};

#include "Pointer64.inl"

X_NAMESPACE_END

#endif // !X_POINTER_64_H_