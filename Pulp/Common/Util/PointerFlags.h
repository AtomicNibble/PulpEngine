#pragma once

#ifndef X_POINTERWITHBITS_H_
#define X_POINTERWITHBITS_H_

X_NAMESPACE_BEGIN(core)

template<class T, uintptr_t BIT_COUNT>
class PointerFlags
{
    ///  bit-mask to get actual pointer
public:
    static const uintptr_t BIT_MASK = (1ul << BIT_COUNT) - 1ul;
    static const uintptr_t BIT_COUNT = BIT_COUNT;

public:
    X_INLINE PointerFlags(void);
    X_INLINE PointerFlags(T* ptr);
    X_INLINE PointerFlags(T* ptr, uintptr_t bits);
    X_INLINE PointerFlags(const PointerFlags<T, BIT_COUNT>& rhs);

    X_INLINE void CopyPointer(const T* ptr);
    X_INLINE void CopyPointer(const T* ptr, uintptr_t bits);
    X_INLINE void CopyPointer(const PointerFlags<T, BIT_COUNT>& rhs);

    X_INLINE uintptr_t GetBits(void) const;

    X_INLINE void SetBits(uintptr_t bits);

    template<uintptr_t BIT>
    X_INLINE bool IsBitSet(void) const;

    template<uintptr_t BIT>
    X_INLINE void SetBit(void);

    template<uintptr_t BIT>
    X_INLINE void ClearBit(void);

    X_INLINE T& operator*(void);
    X_INLINE const T& operator*(void) const;

    X_INLINE T* operator->(void);
    X_INLINE const T* operator->(void) const;

    // since i'm returning by value here i can make return non const.
    // this way we can get none const pointer from const pointerflags.
    X_INLINE operator T*(void) const;

private:
    X_INLINE T* GetRawPointer(void) const;

    X_NO_ASSIGN(PointerFlags);

    T* pointer_;
};

#include "PointerFlags.inl"

X_NAMESPACE_END

#endif // X_POINTERWITHBITS_H_
