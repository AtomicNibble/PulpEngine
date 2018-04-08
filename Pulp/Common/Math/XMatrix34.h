#pragma once

#ifndef _X_MATH_MATRIX_34_H_
#define _X_MATH_MATRIX_34_H_

#include "XQuat.h"
#include "XMatrix33.h"

template<typename T>
class Matrix34
{
public:
    typedef core::StackString<192, char> Description;

    typedef T TYPE;
    typedef T value_type;
    //
    static const size_t DIM = 3;
    static const size_t DIM_SQ = DIM * (DIM + 1);
    static const size_t MEM_LEN = sizeof(T) * DIM_SQ;
    static const T EPSILON;

    X_PUSH_WARNING_LEVEL(3)
    union
    {
        T m[12];
        struct
        {
            // needs changing since we are 4 wide.
            T m00, m10, m20;
            T m01, m11, m21;
            T m02, m12, m22;
            T m03, m13, m23;
        };
        // [Cols][Rows]
        T mcols[4][3];
    };
    X_POP_WARNING_LEVEL

    // i think i must hate myself.
    // having to write UT for all these is fun fun xD
    Matrix34();
    explicit Matrix34(T s);
    Matrix34(const T* dt, bool srcIsRowMajor = false);
    Matrix34(const Matrix22<T>& src);
    Matrix34(const Matrix33<T>& m);
    Matrix34(const Matrix33<T>& m, const Vec3<T>& t);
    Matrix34(const Matrix34<T>& m);

    Matrix34(const Vec3<T>& vx, const Vec3<T>& vy, const Vec3<T>& vz);
    Matrix34(const Vec3<T>& vx, const Vec3<T>& vy, const Vec3<T>& vz, const Vec3<T>& t);

    template<typename FromT>
    Matrix34(const Matrix34<FromT>& m);

    Matrix34(const Matrix44<T>& m);
    Matrix34(T d0, T d1, T d2, T d3, T d4, T d5, T d6, T d7, T d8, T d9, T d10, T d11, bool srcIsRowMajor = false);
    explicit Matrix34(const Quat<T>& q);

    // asign me you melon
    X_INLINE Matrix34& operator=(const Matrix34& rhs);
    X_INLINE Matrix34<T>& operator=(const Matrix33<T>& rhs);
    X_INLINE Matrix34<T>& operator=(const Matrix22<T>& rhs);

    X_INLINE bool equalCompare(const Matrix34<T>& rhs, T epsilon) const;
    X_INLINE bool operator==(const Matrix34<T>& rhs) const
    {
        return equalCompare(rhs, EPSILON);
    }
    X_INLINE bool operator!=(const Matrix34<T>& rhs) const
    {
        return !(*this == rhs);
    }

    X_INLINE Matrix34<T>& operator*=(const Matrix34<T>& rhs);
    X_INLINE Matrix34<T>& operator+=(const Matrix34<T>& rhs);
    X_INLINE Matrix34<T>& operator-=(const Matrix34<T>& rhs);

    X_INLINE Matrix34<T>& operator*=(T rhs);
    X_INLINE Matrix34<T>& operator/=(T rhs);
    X_INLINE Matrix34<T>& operator+=(T rhs);
    X_INLINE Matrix34<T>& operator-=(T rhs);

    X_INLINE const Matrix34<T> operator*(const Matrix34<T>& rhs) const;
    X_INLINE const Matrix34<T> operator+(const Matrix34<T>& rhs) const;
    X_INLINE const Matrix34<T> operator-(const Matrix34<T>& rhs) const;

    // post-multiplies column vector [rhs.x rhs.y rhs.z 1] and divides by w
    X_INLINE const Vec3<T> operator*(const Vec3<T>& rhs) const;

    // post-multiplies column vector [rhs.x rhs.y rhs.z rhs.w]
    X_INLINE const Vec4<T> operator*(const Vec4<T>& rhs) const;

    X_INLINE const Matrix34<T> operator*(T rhs) const;
    X_INLINE const Matrix34<T> operator/(T rhs) const;
    X_INLINE const Matrix34<T> operator+(T rhs) const;
    X_INLINE const Matrix34<T> operator-(T rhs) const;

    // Accessors
    X_INLINE T& at(int row, int col);
    X_INLINE const T& at(int row, int col) const;

    // set
    X_INLINE void set(const T* dt, bool srcIsRowMajor = false);
    X_INLINE void set(T d0, T d1, T d2, T d3, T d4, T d5, T d6, T d7, T d8, T d9, T d10, T d11, bool srcIsRowMajor = false);

    // shizz
    X_INLINE Vec3<T> getColumn(int col) const;
    X_INLINE void setColumn(int col, const Vec3<T>& v);

    X_INLINE Vec3<T> getRow(int row) const;
    X_INLINE void setRow(int row, const Vec3<T>& v);

    X_INLINE void getColumns(Vec3<T>* c0, Vec3<T>* c1, Vec3<T>* c2, Vec3<T>* c3) const;
    X_INLINE void setColumns(const Vec3<T>& c0, const Vec3<T>& c1, const Vec3<T>& c2, const Vec3<T>& c3);

    X_INLINE void getRows(Vec3<T>* r0, Vec3<T>* r1, Vec3<T>* r2) const;
    X_INLINE void setRows(const Vec3<T>& r0, const Vec3<T>& r1, const Vec3<T>& r2);

    // returns a sub-matrix starting at row, col
    X_INLINE Matrix22<T> subMatrix22(int row, int col) const;
    X_INLINE Matrix33<T> subMatrix33(int row, int col) const;

    X_INLINE void setToNull();
    X_INLINE void setToIdentity();

    T determinant() const;
    T trace() const;

    Matrix34<T> diagonal() const;

    Matrix34<T> lowerTriangular() const;
    Matrix34<T> upperTriangular() const;

    void transpose();
    Matrix34<T> transposed() const;

    void invert(T epsilon = EPSILON)
    {
        *this = inverted(epsilon);
    }
    Matrix34<T> inverted(T epsilon = EPSILON) const;

    // pre-multiplies row vector v - no divide by w
    Vec3<T> preMultiply(const Vec3<T>& v) const;

    // post-multiplies column vector v - no divide by w
    Vec3<T> postMultiply(const Vec3<T>& v) const;

    // post-multiplies column vector [rhs.x rhs.y rhs.z]
    Vec3<T> transformVec(const Vec3<T>& v) const
    {
        return postMultiply(v);
    }

    // rotate by radians on axis (conceptually, rotate is before 'this')
    template<template<typename> class VecT>
    void rotate(const VecT<T>& axis, T radians)
    {
        *this *= Matrix34<T>::createRotation(axis, radians);
    }
    // rotate by eulerRadians - Euler angles in radians (conceptually, rotate is before 'this')
    template<template<typename> class VecT>
    void rotate(const VecT<T>& eulerRadians)
    {
        *this *= Matrix34<T>::createRotation(eulerRadians);
    }
    // rotate by matrix derives rotation matrix using from, to, worldUp	(conceptually, rotate is before 'this')
    template<template<typename> class VecT>
    void rotate(const VecT<T>& from, const VecT<T>& to, const VecT<T>& worldUp)
    {
        *this *= Matrix34<T>::createRotation(from, to, worldUp);
    }

    // transposes rotation sub-matrix and inverts translation
    Matrix34<T> invertTransform() const;

    // returns the translation values from the last column
    Vec3<T> getTranslate() const
    {
        return Vec3<T>(m03, m13, m23);
    }
    // sets the translation values in the last column
    void setTranslate(const Vec3<T>& v)
    {
        m03 = v.x;
        m13 = v.y;
        m23 = v.z;
    }
    void setTranslate(const Vec4<T>& v)
    {
        setTranslate(v.xyz());
    }

    void setRotation(const Matrix33<T>& rotation);

    // multiplies the current matrix by the scale matrix derived from supplies parameters
    void scale(T s)
    {
        Matrix44<T> op = createScale(s);
        Matrix44<T> mat = *this;
        *this = op * mat;
    }
    void scale(const Vec2<T>& v)
    {
        *this *= createScale(v);
    }
    void scale(const Vec3<T>& v)
    {
        *this *= createScale(v);
    }
    void scale(const Vec4<T>& v)
    {
        *this *= createScale(v);
    }

    const char* toString(Description& desc) const;


    // creates rotation matrix
    static Matrix34<T> createRotation(const Vec3<T>& axis, T radians);
    static Matrix34<T> createRotation(const Vec3<T>& from, const Vec3<T>& to, const Vec3<T>& worldUp);
    // equivalent to rotate( zAxis, z ), then rotate( yAxis, y ) then rotate( xAxis, x )
    static Matrix34<T> createRotation(const Vec3<T>& eulerRadians);

    // creates scale matrix
    static Matrix34<T> createScale(T s);
    static Matrix34<T> createScale(const Vec2<T>& v);
    static Matrix34<T> createScale(const Vec3<T>& v);
    static Matrix34<T> createScale(const Vec4<T>& v);

    // creates translation matrix
    static Matrix34<T> createTranslation(const Vec3<T>& v);
    static Matrix34<T> createTranslation(const Vec4<T>& v)
    {
        return createTranslation(v.xyz());
    }

    // creates a rotation matrix with z-axis aligned to targetDir
    static Matrix34<T> alignZAxisWithTarget(Vec3<T> targetDir, Vec3<T> upDir);

    // returns an identity matrix
    static Matrix34<T> identity()
    {
        return Matrix34(1, 0, 0,
            0, 1, 0,
            0, 0, 1,
            0, 0, 0);
    }
    // returns 1 filled matrix
    static Matrix34<T> one()
    {
        return Matrix34((T)1);
    }
    // returns 0 filled matrix
    static Matrix34<T> zero()
    {
        return Matrix34((T)0);
    }
};

typedef Matrix34<float32_t> Matrix34f;
typedef Matrix34<float64_t> Matrix34d;


template<typename T>
const T Matrix34<T>::EPSILON = math<T>::CMP_EPSILON;

#include "XMatrix34.inl"

#endif // !_X_MATH_MATRIX_34_H_
