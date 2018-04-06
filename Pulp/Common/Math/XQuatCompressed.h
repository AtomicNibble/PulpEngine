#pragma once

#ifndef _X_MATH_QUAT_COMPRESSED_H_
#define _X_MATH_QUAT_COMPRESSED_H_

#include "XQuat.h"

template<typename T>
class XQuatCompressed
{
public:
    typedef T TYPE;
    typedef T value_type;
    typedef int16_t comp_type;

public:
    XQuatCompressed();
    explicit XQuatCompressed(const Quat<T>& q);
    explicit XQuatCompressed(const Matrix33<T>& m);
    XQuatCompressed(T aW, T x, T y, T z);

    void set(const Quat<T>& q);
    void set(const Matrix33<T>& m);
    void set(T aW, T x, T y, T z);

    Quat<T> asQuat(void) const;
    Matrix33<T> asMatrix33(void) const;

    // Operators
    XQuatCompressed<T>& operator=(const XQuatCompressed<T>& rhs);

    bool operator==(const XQuatCompressed<T>& rhs) const;
    bool operator!=(const XQuatCompressed<T>& rhs) const;

    X_INLINE T& operator[](size_t i);
    X_INLINE const T& operator[](size_t i) const;

    X_INLINE static XQuatCompressed<T> identity(void);

private:
    Vec3<comp_type> v_;
    comp_type w_;
};

typedef XQuatCompressed<float32_t> XQuatCompressedf;
typedef XQuatCompressed<float64_t> XQuatCompressedd;

#include "XQuatCompressed.inl"

#endif // !_X_MATH_QUAT_COMPRESSED_H_
