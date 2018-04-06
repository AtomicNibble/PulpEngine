

#ifndef _X_TYPES_H_
#include "Types.h"
#endif

template<typename T>
X_INLINE Matrix34<T>::Matrix34()
{
    setToIdentity();

    // I think when using the class I would assume defualt construction is
    // identity.
    // I also set to identity for Mat22,33,44
#if X_DEBUG == 1 && 0
    memset(m, 0xff, MEM_LEN);
#endif
}

template<typename T>
Matrix34<T>::Matrix34(T s)
{
    for (int i = 0; i < DIM_SQ; ++i) {
        m[i] = s;
    }
}

template<typename T>
X_INLINE Matrix34<T>::Matrix34(const T* dt, bool srcIsRowMajor)
{
    set(dt, srcIsRowMajor);
}

template<typename T>
X_INLINE Matrix34<T>::Matrix34(const Matrix22<T>& src)
{
    setToIdentity();
    m00 = src.m00;
    m01 = src.m01;
    m10 = src.m10;
    m11 = src.m11;
}

// all the construction options you want baby!
template<typename T>
X_INLINE Matrix34<T>::Matrix34(const Matrix33<T>& m)
{
    m00 = T(m.m00);
    m01 = T(m.m01);
    m02 = T(m.m02); // m03 = 0;
    m10 = T(m.m10);
    m11 = T(m.m11);
    m12 = T(m.m12); // m13 = 0;
    m20 = T(m.m20);
    m21 = T(m.m21);
    m22 = T(m.m22); //m23 = 0;

    m03 = T(0);
    m13 = T(0);
    m23 = T(0);
}

template<typename T>
X_INLINE Matrix34<T>::Matrix34(const Matrix33<T>& m, const Vec3<T>& t)
{
    m00 = T(m.m00);
    m01 = T(m.m01);
    m02 = T(m.m02); // m03 = T(t.x);
    m10 = T(m.m10);
    m11 = T(m.m11);
    m12 = T(m.m12); // m13 = T(t.y);
    m20 = T(m.m20);
    m21 = T(m.m21);
    m22 = T(m.m22); // m23 = T(t.z);

    m03 = T(t.x);
    m13 = T(t.y);
    m23 = T(t.z);
}

template<typename T>
X_INLINE Matrix34<T>::Matrix34(const Matrix34<T>& m)
{
    m00 = T(m.m00);
    m01 = T(m.m01);
    m02 = T(m.m02); //m03 = T(m.m03);
    m10 = T(m.m10);
    m11 = T(m.m11);
    m12 = T(m.m12); //m13 = T(m.m13);
    m20 = T(m.m20);
    m21 = T(m.m21);
    m22 = T(m.m22); //m23 = T(m.m23);

    m03 = T(m.m03);
    m13 = T(m.m13);
    m23 = T(m.m23);
}

template<typename T>
X_INLINE Matrix34<T>::Matrix34(const Vec3<T>& vx, const Vec3<T>& vy, const Vec3<T>& vz)
{
    m00 = vx.x;
    m01 = vy.x;
    m02 = vz.x;
    m10 = vx.y;
    m11 = vy.y;
    m12 = vz.y;
    m20 = vx.z;
    m21 = vy.z;
    m22 = vz.z;

    m03 = m13 = m23 = (T)0;
}

template<typename T>
X_INLINE Matrix34<T>::Matrix34(const Vec3<T>& vx, const Vec3<T>& vy, const Vec3<T>& vz,
    const Vec3<T>& t)
{
    m00 = vx.x;
    m01 = vy.x;
    m02 = vz.x;
    m10 = vx.y;
    m11 = vy.y;
    m12 = vz.y;
    m20 = vx.z;
    m21 = vy.z;
    m22 = vz.z;

    m03 = t.x;
    m13 = t.y;
    m23 = t.z;
}

template<typename T>
template<typename FromT>
X_INLINE Matrix34<T>::Matrix34(const Matrix34<FromT>& m)
{
    m00 = T(m.m00);
    m01 = T(m.m01);
    m02 = T(m.m02); //m03 = T(m.m03);
    m10 = T(m.m10);
    m11 = T(m.m11);
    m12 = T(m.m12); //m13 = T(m.m13);
    m20 = T(m.m20);
    m21 = T(m.m21);
    m22 = T(m.m22); //m23 = T(m.m23);

    m03 = T(m.m03);
    m13 = T(m.m13);
    m23 = T(m.m23);
}

template<typename T>
X_INLINE Matrix34<T>::Matrix34(const Matrix44<T>& m)
{
    m00 = T(m.m00);
    m01 = T(m.m01);
    m02 = T(m.m02); // m03 = T(m.m03);
    m10 = T(m.m10);
    m11 = T(m.m11);
    m12 = T(m.m12); // m13 = T(m.m13);
    m20 = T(m.m20);
    m21 = T(m.m21);
    m22 = T(m.m22); // m23 = T(m.m23);

    m03 = T(m.m03);
    m13 = T(m.m13);
    m23 = T(m.m23);
}

template<typename T>
X_INLINE Matrix34<T>::Matrix34(T d0, T d1, T d2, T d3, T d4, T d5, T d6,
    T d7, T d8, T d9, T d10, T d11, bool srcIsRowMajor)
{
    set(d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11, srcIsRowMajor);
}

template<typename T>
X_INLINE Matrix34<T>::Matrix34(const Quat<T>& q)
{
    *this = q.toMatrix33();
}

// asign me you melon
template<typename T>
X_INLINE Matrix34<T>& Matrix34<T>::operator=(const Matrix34<T>& rhs)
{
    memcpy(m, rhs.m, MEM_LEN);
    return *this;
}

template<typename T>
X_INLINE Matrix34<T>& Matrix34<T>::operator=(const Matrix33<T>& rhs)
{
    setToIdentity();
    m00 = rhs.m00;
    m01 = rhs.m01;
    m02 = rhs.m02;
    m10 = rhs.m10;
    m11 = rhs.m11;
    m12 = rhs.m12;
    m20 = rhs.m20;
    m21 = rhs.m21;
    m22 = rhs.m22;
    return *this;
}

template<typename T>
X_INLINE Matrix34<T>& Matrix34<T>::operator=(const Matrix22<T>& rhs)
{
    setToIdentity();
    m00 = rhs.m00;
    m01 = rhs.m01;
    m10 = rhs.m10;
    m11 = rhs.m11;
    return *this;
}

template<typename T>
X_INLINE bool Matrix34<T>::equalCompare(const Matrix34<T>& rhs, T epsilon) const
{
    for (int i = 0; i < DIM_SQ; ++i) {
        if (math<T>::abs(m[i] - rhs.m[i]) >= epsilon)
            return false;
    }
    return true;
}

template<typename T>
X_INLINE Matrix34<T>& Matrix34<T>::operator*=(const Matrix34<T>& rhs)
{
    Matrix34<T> ret;

    ret.m00 = m00 * rhs.m00 + m01 * rhs.m10 + m02 * rhs.m20;
    ret.m10 = m10 * rhs.m00 + m11 * rhs.m10 + m12 * rhs.m20;
    ret.m20 = m20 * rhs.m00 + m21 * rhs.m10 + m22 * rhs.m20;

    ret.m01 = m00 * rhs.m01 + m01 * rhs.m11 + m02 * rhs.m21;
    ret.m11 = m10 * rhs.m01 + m11 * rhs.m11 + m12 * rhs.m21;
    ret.m21 = m20 * rhs.m01 + m21 * rhs.m11 + m22 * rhs.m21;

    ret.m02 = m00 * rhs.m02 + m01 * rhs.m12 + m02 * rhs.m22;
    ret.m12 = m10 * rhs.m02 + m11 * rhs.m12 + m12 * rhs.m22;
    ret.m22 = m20 * rhs.m02 + m21 * rhs.m12 + m22 * rhs.m22;

    ret.m03 = m00 * rhs.m03 + m01 * rhs.m13 + m02 * rhs.m23 + m03;
    ret.m13 = m10 * rhs.m03 + m11 * rhs.m13 + m12 * rhs.m23 + m13;
    ret.m23 = m20 * rhs.m03 + m21 * rhs.m13 + m22 * rhs.m23 + m23;

    for (int i = 0; i < DIM_SQ; ++i) {
        m[i] = ret.m[i];
    }

    return *this;
}

template<typename T>
X_INLINE Matrix34<T>& Matrix34<T>::operator+=(const Matrix34<T>& rhs)
{
    for (int i = 0; i < DIM_SQ; ++i) {
        m[i] += rhs.m[i];
    }
    return *this;
}

template<typename T>
X_INLINE Matrix34<T>& Matrix34<T>::operator-=(const Matrix34<T>& rhs)
{
    for (int i = 0; i < DIM_SQ; ++i) {
        m[i] -= rhs.m[i];
    }
    return *this;
}

template<typename T>
X_INLINE Matrix34<T>& Matrix34<T>::operator*=(T rhs)
{
    for (int i = 0; i < DIM_SQ; ++i) {
        m[i] *= rhs;
    }
    return *this;
}

template<typename T>
X_INLINE Matrix34<T>& Matrix34<T>::operator/=(T rhs)
{
    T invS = (T)1 / rhs;
    for (int i = 0; i < DIM_SQ; ++i) {
        m[i] *= invS;
    }
    return *this;
}

template<typename T>
X_INLINE Matrix34<T>& Matrix34<T>::operator+=(T rhs)
{
    for (int i = 0; i < DIM_SQ; ++i) {
        m[i] += rhs;
    }
    return *this;
}

template<typename T>
X_INLINE Matrix34<T>& Matrix34<T>::operator-=(T rhs)
{
    for (int i = 0; i < DIM_SQ; ++i) {
        m[i] -= rhs;
    }
    return *this;
}

template<typename T>
X_INLINE const Matrix34<T> Matrix34<T>::operator*(const Matrix34<T>& rhs) const
{
    Matrix34<T> ret;

    ret.m[0] = m[0] * rhs.m[0] + m[3] * rhs.m[1] + m[6] * rhs.m[2];
    ret.m[1] = m[1] * rhs.m[0] + m[4] * rhs.m[1] + m[7] * rhs.m[2];
    ret.m[2] = m[2] * rhs.m[0] + m[5] * rhs.m[1] + m[8] * rhs.m[2];

    ret.m[3] = m[0] * rhs.m[3] + m[3] * rhs.m[4] + m[6] * rhs.m[5];
    ret.m[4] = m[1] * rhs.m[3] + m[4] * rhs.m[4] + m[7] * rhs.m[5];
    ret.m[5] = m[2] * rhs.m[3] + m[5] * rhs.m[4] + m[8] * rhs.m[5];

    ret.m[6] = m[0] * rhs.m[6] + m[3] * rhs.m[7] + m[6] * rhs.m[8];
    ret.m[7] = m[1] * rhs.m[6] + m[4] * rhs.m[7] + m[7] * rhs.m[8];
    ret.m[8] = m[2] * rhs.m[6] + m[5] * rhs.m[7] + m[8] * rhs.m[8];

    // trans
    ret.m[9] = m[0] * rhs.m[9] + m[3] * rhs.m[10] + m[6] * rhs.m[11] + m[9];
    ret.m[10] = m[1] * rhs.m[9] + m[4] * rhs.m[10] + m[7] * rhs.m[11] + m[10];
    ret.m[11] = m[2] * rhs.m[9] + m[5] * rhs.m[10] + m[8] * rhs.m[11] + m[11];

    return ret;
}

template<typename T>
X_INLINE const Matrix34<T> Matrix34<T>::operator+(const Matrix34<T>& rhs) const
{
    Matrix34<T> ret;
    for (int i = 0; i < DIM_SQ; ++i) {
        ret.m[i] = m[i] + rhs.m[i];
    }
    return ret;
}

template<typename T>
X_INLINE const Matrix34<T> Matrix34<T>::operator-(const Matrix34<T>& rhs) const
{
    Matrix34<T> ret;
    for (int i = 0; i < DIM_SQ; ++i) {
        ret.m[i] = m[i] - rhs.m[i];
    }
    return ret;
}

// post-multiplies column vector [rhs.x rhs.y rhs.z 1] and divides by w
template<typename T>
X_INLINE const Vec3<T> Matrix34<T>::operator*(const Vec3<T>& rhs) const
{
    //	T x = m[0] * rhs.x + m[4] * rhs.y + m[8];
    //	T y = m[1] * rhs.x + m[5] * rhs.y + m[9];
    //	T z = m[2] * rhs.x + m[6] * rhs.y + m[10];
    //	T w = m[3] * rhs.x + m[7] * rhs.y + m[11];

    T x = m00 * rhs.x + m01 * rhs.y + m02 * rhs.z + m03;
    T y = m10 * rhs.x + m11 * rhs.y + m12 * rhs.z + m13;
    T z = m20 * rhs.x + m21 * rhs.y + m22 * rhs.z + m23;

    return Vec3<T>(x, y, z);
    //	return Vec3<T>(x / w, y / w, z / w);
}

// post-multiplies column vector [rhs.x rhs.y rhs.z rhs.w]
template<typename T>
X_INLINE const Vec4<T> Matrix34<T>::operator*(const Vec4<T>& rhs) const
{
    return Vec4<T>(
        m[0] * rhs.x + m[4] * rhs.y + m[8] * rhs.w,
        m[1] * rhs.x + m[5] * rhs.y + m[9] * rhs.w,
        m[2] * rhs.x + m[6] * rhs.y + m[10] * rhs.w,
        m[3] * rhs.x + m[7] * rhs.y + m[11] * rhs.w);
}

template<typename T>
X_INLINE const Matrix34<T> Matrix34<T>::operator*(T rhs) const
{
    Matrix34<T> ret;
    for (int i = 0; i < DIM_SQ; ++i) {
        ret.m[i] = m[i] * rhs;
    }
    return ret;
}

template<typename T>
X_INLINE const Matrix34<T> Matrix34<T>::operator/(T rhs) const
{
    Matrix34<T> ret;
    T s = (T)1 / rhs;
    for (int i = 0; i < DIM_SQ; ++i) {
        ret.m[i] = m[i] * s;
    }
    return ret;
}

template<typename T>
X_INLINE const Matrix34<T> Matrix34<T>::operator+(T rhs) const
{
    Matrix34<T> ret;
    for (int i = 0; i < DIM_SQ; ++i) {
        ret.m[i] = m[i] + rhs;
    }
    return ret;
}

template<typename T>
X_INLINE const Matrix34<T> Matrix34<T>::operator-(T rhs) const
{
    Matrix34<T> ret;
    for (int i = 0; i < DIM_SQ; ++i) {
        ret.m[i] = m[i] - rhs;
    }
    return ret;
}

// Accessors
template<typename T>
X_INLINE T& Matrix34<T>::at(int row, int col)
{
    X_ASSERT(row >= 0 && row < DIM, "row out of range")
    (DIM, row);
    X_ASSERT(col >= 0 && col < DIM + 1, "col out of range")
    (DIM + 1, col);
    return m[col * DIM + row];
}

template<typename T>
X_INLINE const T& Matrix34<T>::at(int row, int col) const
{
    X_ASSERT(row >= 0 && row < DIM, "row out of range")
    (DIM, row);
    X_ASSERT(col >= 0 && col < DIM + 1, "col out of range")
    (DIM + 1, col);
    return m[col * DIM + row];
}

template<typename T>
X_INLINE void Matrix34<T>::set(const T* dt, bool srcIsRowMajor)
{
    if (srcIsRowMajor) {
        m[0] = dt[0];
        m[3] = dt[1];
        m[6] = dt[2];
        m[1] = dt[3];
        m[4] = dt[4];
        m[7] = dt[5];
        m[2] = dt[6];
        m[5] = dt[7];
        m[8] = dt[8];

        m[9] = dt[9];
        m[10] = dt[10];
        m[11] = dt[11];
    }
    else {
        std::memcpy(m, dt, MEM_LEN);
    }
}

template<typename T>
X_INLINE void Matrix34<T>::set(T d0, T d1, T d2, T d3, T d4, T d5, T d6,
    T d7, T d8, T d9, T d10, T d11, bool srcIsRowMajor)
{
    if (srcIsRowMajor) {
        m[0] = d0;
        m[3] = d1;
        m[6] = d2;
        m[1] = d4;
        m[4] = d5;
        m[7] = d6;
        m[2] = d8;
        m[5] = d9;
        m[8] = d10;

        // trans
        m[9] = d3;
        m[10] = d7;
        m[11] = d11;
    }
    else {
        m[0] = d0;
        m[3] = d3;
        m[6] = d6;
        m[1] = d1;
        m[4] = d4;
        m[7] = d7;
        m[2] = d2;
        m[5] = d5;
        m[8] = d8;

        // trans
        m[9] = d9;
        m[10] = d10;
        m[11] = d11;
    }
}

template<typename T>
X_INLINE Vec3<T> Matrix34<T>::getColumn(int col) const
{
    size_t i = col * DIM;
    return Vec3<T>(
        m[i + 0],
        m[i + 1],
        m[i + 2]);
}

template<typename T>
X_INLINE void Matrix34<T>::setColumn(int col, const Vec3<T>& v)
{
    size_t i = col * DIM;
    m[i + 0] = v.x;
    m[i + 1] = v.y;
    m[i + 2] = v.z;
}

template<typename T>
X_INLINE Vec3<T> Matrix34<T>::getRow(int row) const
{
    return Vec3<T>(
        m[row + 0],
        m[row + 3],
        m[row + 6]
        //	m[row + 9]
    );
}

template<typename T>
X_INLINE void Matrix34<T>::setRow(int row, const Vec3<T>& v)
{
    m[row + 0] = v.x;
    m[row + 3] = v.y;
    m[row + 6] = v.z;
    //	m[row + 12] = v.w;
}

template<typename T>
X_INLINE void Matrix34<T>::getColumns(Vec3<T>* c0, Vec3<T>* c1, Vec3<T>* c2, Vec3<T>* c3) const
{
    *c0 = getColumn(0);
    *c1 = getColumn(1);
    *c2 = getColumn(2);
    *c3 = getColumn(3);
}

template<typename T>
X_INLINE void Matrix34<T>::setColumns(const Vec3<T>& c0, const Vec3<T>& c1,
    const Vec3<T>& c2, const Vec3<T>& c3)
{
    setColumn(0, c0);
    setColumn(1, c1);
    setColumn(2, c2);
    setColumn(3, c3);
}

template<typename T>
X_INLINE void Matrix34<T>::getRows(Vec3<T>* r0, Vec3<T>* r1, Vec3<T>* r2) const
{
    *r0 = getRow(0);
    *r1 = getRow(1);
    *r2 = getRow(2);
}

template<typename T>
X_INLINE void Matrix34<T>::setRows(const Vec3<T>& r0, const Vec3<T>& r1, const Vec3<T>& r2)
{
    setRow(0, r0);
    setRow(1, r1);
    setRow(2, r2);
}

// returns a sub-matrix starting at row, col
template<typename T>
X_INLINE Matrix22<T> Matrix34<T>::subMatrix22(int row, int col) const
{
    Matrix22<T> ret;
    ret.setToNull();

    for (int i = 0; i < 2; ++i) {
        int r = row + i;
        if (r >= 4) {
            continue;
        }
        for (int j = 0; j < 2; ++j) {
            int c = col + j;
            if (c >= 4) {
                continue;
            }
            ret.at(i, j) = at(r, c);
        }
    }

    return ret;
}

template<typename T>
X_INLINE Matrix33<T> Matrix34<T>::subMatrix33(int row, int col) const
{
    Matrix33<T> ret;
    ret.setToNull();

    for (int i = 0; i < 3; ++i) {
        int r = row + i;
        if (r >= 4) {
            continue;
        }
        for (int j = 0; j < 3; ++j) {
            int c = col + j;
            if (c >= 4) {
                continue;
            }
            ret.at(i, j) = at(r, c);
        }
    }

    return ret;
}

template<typename T>
X_INLINE void Matrix34<T>::setToNull()
{
    memset(m, 0, MEM_LEN);
}

template<typename T>
X_INLINE void Matrix34<T>::setToIdentity()
{
    m00 = 1.0f;
    m01 = 0.0f;
    m02 = 0.0f; //m03 = 0.0f;
    m10 = 0.0f;
    m11 = 1.0f;
    m12 = 0.0f; //m13 = 0.0f;
    m20 = 0.0f;
    m21 = 0.0f;
    m22 = 1.0f; //m23 = 0.0f;

    m03 = 0.0f;
    m13 = 0.0f;
    m23 = 0.0f;
}

template<typename T>
X_INLINE T Matrix34<T>::determinant() const
{
    T co00 = m[4] * m[8] - m[5] * m[7];
    T co10 = m[5] * m[6] - m[3] * m[8];
    T co20 = m[3] * m[7] - m[4] * m[6];

    T det = m[0] * co00 + m[1] * co10 + m[2] * co20;

    return det;
}

template<typename T>
X_INLINE T Matrix34<T>::trace() const
{
    return m00 + m11 + m22;
}

template<typename T>
X_INLINE Matrix34<T> Matrix34<T>::diagonal() const
{
    Matrix34 ret;
    ret.m00 = m00;
    ret.m01 = 0;
    ret.m02 = 0;
    ret.m10 = 0;
    ret.m11 = m11;
    ret.m12 = 0;
    ret.m20 = 0;
    ret.m21 = 0;
    ret.m22 = m22;

    // trans
    ret.m03 = m03;
    ret.m13 = m13;
    ret.m23 = m23;
    return ret;
}

template<typename T>
X_INLINE Matrix34<T> Matrix34<T>::lowerTriangular() const
{
    Matrix34 ret;
    ret.m00 = m00;
    ret.m01 = 0;
    ret.m02 = 0;
    ret.m10 = m10;
    ret.m11 = m11;
    ret.m12 = 0;
    ret.m20 = m20;
    ret.m21 = m21;
    ret.m22 = m22;

    // trans
    ret.m03 = m03;
    ret.m13 = m13;
    ret.m23 = m23;
    return ret;
}

template<typename T>
X_INLINE Matrix34<T> Matrix34<T>::upperTriangular() const
{
    Matrix34 ret;
    ret.m00 = m00;
    ret.m01 = m01;
    ret.m02 = m02;
    ret.m10 = 0;
    ret.m11 = m11;
    ret.m12 = m12;
    ret.m20 = 0;
    ret.m21 = 0;
    ret.m22 = m22;

    // trans
    ret.m03 = m03;
    ret.m13 = m13;
    ret.m23 = m23;
    return ret;
}

template<typename T>
X_INLINE void Matrix34<T>::transpose()
{
    T t;
    //	Vec3<T> v = getTranslate();
    t = m01;
    m01 = m10;
    m10 = t; // m03 = -v.x*m00 - v.y*m01 - v.z*m20;
    t = m02;
    m02 = m20;
    m20 = t; // m13 = -v.x*m10 - v.y*m11 - v.z*m21;
    t = m12;
    m12 = m21;
    m21 = t; // m23 = -v.x*m20 - v.y*m21 - v.z*m22;
}

template<typename T>
X_INLINE Matrix34<T> Matrix34<T>::transposed() const
{
    return Matrix34<T>(
        m[0], m[3], m[6],
        m[1], m[4], m[7],
        m[2], m[5], m[8],
        m[9], m[10], m[11]);
}

template<typename T>
X_INLINE Matrix34<T> Matrix34<T>::inverted(T epsilon) const
{
    Matrix34<T> inv((T)0);

    // Compute the adjoint.
    inv.m[0] = m[4] * m[8] - m[5] * m[7];
    inv.m[1] = m[2] * m[7] - m[1] * m[8];
    inv.m[2] = m[1] * m[5] - m[2] * m[4];
    inv.m[3] = m[5] * m[6] - m[3] * m[8];
    inv.m[4] = m[0] * m[8] - m[2] * m[6];
    inv.m[5] = m[2] * m[3] - m[0] * m[5];
    inv.m[6] = m[3] * m[7] - m[4] * m[6];
    inv.m[7] = m[1] * m[6] - m[0] * m[7];
    inv.m[8] = m[0] * m[4] - m[1] * m[3];

    T det = m[0] * inv.m[0] + m[1] * inv.m[3] + m[2] * inv.m[6];

    if (fabs(det) > epsilon) {
        T invDet = (T)1 / det;
        inv.m[0] *= invDet;
        inv.m[1] *= invDet;
        inv.m[2] *= invDet;
        inv.m[3] *= invDet;
        inv.m[4] *= invDet;
        inv.m[5] *= invDet;
        inv.m[6] *= invDet;
        inv.m[7] *= invDet;
        inv.m[8] *= invDet;
    }

    // set translate
    inv.setTranslate(getTranslate());

    return inv;
}

template<typename T>
X_INLINE Vec3<T> Matrix34<T>::preMultiply(const Vec3<T>& v) const
{
    return Vec3<T>(
        v.x * m00 + v.y * m10 + v.z * m20,
        v.x * m01 + v.y * m11 + v.z * m21,
        v.x * m02 + v.y * m12 + v.z * m22);
}

// post-multiplies column vector v - no divide by w
template<typename T>
X_INLINE Vec3<T> Matrix34<T>::postMultiply(const Vec3<T>& v) const
{
    return Vec3<T>(
        m00 * v.x + m01 * v.y + m02 * v.z,
        m10 * v.x + m11 * v.y + m12 * v.z,
        m20 * v.x + m21 * v.y + m22 * v.z);
}

template<typename T>
Matrix34<T> Matrix34<T>::invertTransform() const
{
    Matrix34<T> ret;

    // transpose rotation part
    for (int i = 0; i < DIM; i++) {
        for (int j = 0; j < DIM; j++) {
            ret.at(j, i) = at(i, j);
        }
    }
#if 1
    // dot product each.
    Vec3<T> cur = getTranslate();

    Vec3<T> trans(
        getColumn(0).dot(cur),
        getColumn(1).dot(cur),
        getColumn(2).dot(cur));

    ret.setTranslate(trans);
#else
    ret.setTranslate(-getTranslate());
#endif
    return ret;
}

template<typename T>
void Matrix34<T>::setRotation(const Matrix33<T>& rotation)
{
    m00 = rotation.m00;
    m01 = rotation.m01;
    m02 = rotation.m02;
    m10 = rotation.m10;
    m11 = rotation.m11;
    m12 = rotation.m12;
    m20 = rotation.m20;
    m21 = rotation.m21;
    m22 = rotation.m22;
}

template<typename T>
Matrix34<T> Matrix34<T>::createRotation(const Vec3<T>& from, const Vec3<T>& to, const Vec3<T>& worldUp)
{
    // The goal is to obtain a rotation matrix that takes
    // "fromDir" to "toDir".  We do this in two steps and
    // compose the resulting rotation matrices;
    //    (a) rotate "fromDir" into the z-axis
    //    (b) rotate the z-axis into "toDir"

    // The from direction must be non-zero; but we allow zero to and up dirs.
    if (from.lengthSquared() == 0) {
        return Matrix34<T>();
    }
    else {
        Matrix34<T> zAxis2FromDir = alignZAxisWithTarget(from, Vec3<T>::yAxis());
        Matrix34<T> fromDir2zAxis = zAxis2FromDir.transposed();
        Matrix34<T> zAxis2ToDir = alignZAxisWithTarget(to, worldUp);
        return fromDir2zAxis * zAxis2ToDir;
    }
}

template<typename T>
Matrix34<T> Matrix34<T>::createRotation(const Vec3<T>& axis, T angle)
{
    Vec3<T> unit(axis.normalized());
    T sine = math<T>::sin(angle);
    T cosine = math<T>::cos(angle);

    Matrix34<T> ret;

    ret.m[0] = unit.x * unit.x * (1 - cosine) + cosine;
    ret.m[1] = unit.x * unit.y * (1 - cosine) + unit.z * sine;
    ret.m[2] = unit.x * unit.z * (1 - cosine) - unit.y * sine;

    ret.m[3] = unit.x * unit.y * (1 - cosine) - unit.z * sine;
    ret.m[4] = unit.y * unit.y * (1 - cosine) + cosine;
    ret.m[5] = unit.y * unit.z * (1 - cosine) + unit.x * sine;

    ret.m[6] = unit.x * unit.z * (1 - cosine) + unit.y * sine;
    ret.m[7] = unit.y * unit.z * (1 - cosine) - unit.x * sine;
    ret.m[8] = unit.z * unit.z * (1 - cosine) + cosine;

    ret.m[9] = 0;
    ret.m[10] = 0;
    ret.m[11] = 0;

    return ret;
}

template<typename T>
Matrix34<T> Matrix34<T>::createRotation(const Vec3<T>& eulerRadians)
{
    // The ordering for this is XYZ. In OpenGL, the ordering
    // is the same, but the operations needs to happen in
    // reverse:
    //
    //     glRotatef( eulerRadians.z, 0.0f, 0.0f 1.0f );
    //     glRotatef( eulerRadians.y, 0.0f, 1.0f 0.0f );
    //     glRotatef( eulerRadians.x, 1.0f, 0.0f 0.0f );
    //

    Matrix34<T> ret;
    T cos_rz, sin_rz, cos_ry, sin_ry, cos_rx, sin_rx;

    cos_rx = math<T>::cos(eulerRadians.x);
    cos_ry = math<T>::cos(eulerRadians.y);
    cos_rz = math<T>::cos(eulerRadians.z);

    sin_rx = math<T>::sin(eulerRadians.x);
    sin_ry = math<T>::sin(eulerRadians.y);
    sin_rz = math<T>::sin(eulerRadians.z);

    ret.m[0] = cos_rz * cos_ry;
    ret.m[1] = sin_rz * cos_ry;
    ret.m[2] = -sin_ry;

    ret.m[3] = -sin_rz * cos_rx + cos_rz * sin_ry * sin_rx;
    ret.m[4] = cos_rz * cos_rx + sin_rz * sin_ry * sin_rx;
    ret.m[5] = cos_ry * sin_rx;

    ret.m[6] = sin_rz * sin_rx + cos_rz * sin_ry * cos_rx;
    ret.m[7] = -cos_rz * sin_rx + sin_rz * sin_ry * cos_rx;
    ret.m[8] = cos_ry * cos_rx;

    ret.m[9] = 0;
    ret.m[10] = 0;
    ret.m[11] = 0;

    return ret;
}

// creates scale matrix
template<typename T>
X_INLINE Matrix34<T> Matrix34<T>::createScale(T s)
{
    Matrix44<T> ret;
    ret.setToIdentity();
    ret.at(0, 0) = s;
    ret.at(1, 1) = s;
    ret.at(2, 2) = s;

    return ret;
}

template<typename T>
X_INLINE Matrix34<T> Matrix34<T>::createScale(const Vec2<T>& v)
{
    Matrix44<T> ret;
    ret.setToIdentity();
    ret.at(0, 0) = v.x;
    ret.at(1, 1) = v.y;
    ret.at(2, 2) = 1;

    return ret;
}

template<typename T>
X_INLINE Matrix34<T> Matrix34<T>::createScale(const Vec3<T>& v)
{
    Matrix44<T> ret;
    ret.setToIdentity();
    ret.at(0, 0) = v.x;
    ret.at(1, 1) = v.y;
    ret.at(2, 2) = v.z;

    return ret;
}

template<typename T>
X_INLINE Matrix34<T> Matrix34<T>::createScale(const Vec4<T>& v)
{
    Matrix44<T> ret;
    ret.setToIdentity();
    ret.at(0, 0) = v.x;
    ret.at(1, 1) = v.y;
    ret.at(2, 2) = v.z;

    return ret;
}

template<typename T>
X_INLINE Matrix34<T> Matrix34<T>::createTranslation(const Vec3<T>& v)
{
    Matrix34<T> ret;

    ret.m00 = 1.0f;
    ret.m01 = 0.0f;
    ret.m02 = 0.0f;
    ret.m10 = 0.0f;
    ret.m11 = 1.0f;
    ret.m12 = 0.0f;
    ret.m20 = 0.0f;
    ret.m21 = 0.0f;
    ret.m22 = 1.0f;

    ret.setTranslate(v);
    return ret;
}

template<typename T>
Matrix34<T> Matrix34<T>::alignZAxisWithTarget(Vec3<T> targetDir, Vec3<T> upDir)
{
    // Ensure that the target direction is non-zero.
    if (targetDir.lengthSquared() < EPSILON) {
        // We want to look down the negative z-axis since to match OpenGL.
        targetDir = -Vec3<T>::zAxis();
    }

    // Ensure that the up direction is non-zero.
    if (upDir.lengthSquared() < EPSILON) {
        upDir = Vec3<T>::yAxis();
    }

    // Check for degeneracies.  If the upDir and targetDir are parallel
    // or opposite, then compute a new, arbitrary up direction that is
    // not parallel or opposite to the targetDir.
    if (upDir.cross(targetDir).lengthSquared() == 0) {
        upDir = targetDir.cross(Vec3<T>::xAxis());
        if (upDir.lengthSquared() == 0) {
            upDir = targetDir.cross(Vec3<T>::zAxis());
        }
    }

    // Compute the x-, y-, and z-axis vectors of the new coordinate system.
    Vec3<T> targetPerpDir = targetDir.cross(upDir);
    Vec3<T> targetUpDir = targetPerpDir.cross(targetDir);

    // Rotate the x-axis into targetPerpDir (row 0),
    // rotate the y-axis into targetUpDir   (row 1),
    // rotate the z-axis into targetDir     (row 2).
    Vec3<T> row[3];
    row[0] = targetPerpDir.normalized();
    row[1] = targetUpDir.normalized();
    row[2] = targetDir.normalized();

    Matrix34<T> mat(
        row[0].x, row[0].y, row[0].z,
        row[1].x, row[1].y, row[1].z,
        row[2].x, row[2].y, row[2].z,
        (T)0, (T)0, (T)0);

    return mat;
}