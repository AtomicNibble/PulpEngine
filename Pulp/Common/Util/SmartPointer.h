#pragma once

#ifndef X_SMART_POINTER_H_
#define X_SMART_POINTER_H_

X_NAMESPACE_BEGIN(core)

template<typename T>
class SmartPointer
{
public:
    X_INLINE SmartPointer();
    X_INLINE SmartPointer(T* p);
    X_INLINE SmartPointer(const SmartPointer& oth);
    X_INLINE ~SmartPointer();

    X_INLINE operator T*() const;
    X_INLINE operator const T*() const;
    X_INLINE operator bool() const;
    X_INLINE T& operator*() const;
    X_INLINE T* operator->() const;
    X_INLINE T* get() const;

    X_INLINE void reset();
    X_INLINE void reset(T* p);

    X_INLINE SmartPointer& operator=(T* newP);
    X_INLINE SmartPointer& operator=(const SmartPointer& newP);

    X_INLINE bool operator!() const;
    X_INLINE bool operator==(const T* p2) const;
    X_INLINE bool operator==(T* p2) const;
    X_INLINE bool operator==(const SmartPointer<T>& rhs) const;
    X_INLINE bool operator!=(const T* p2) const;
    X_INLINE bool operator!=(T* p2) const;
    X_INLINE bool operator!=(const SmartPointer& p2) const;
    X_INLINE bool operator<(const T* p2) const;
    X_INLINE bool operator>(const T* p2) const;

    X_INLINE void swap(SmartPointer<T>& oth);

private:
    T* pData_;
};

#include "SmartPointer.inl"

X_NAMESPACE_END

#endif // !X_SMART_POINTER_H_