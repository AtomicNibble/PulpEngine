#pragma once

#ifndef _X_MATH_MATRIX44_H_
#define _X_MATH_MATRIX44_H_

#include "XMath.h"
#include "XVector.h"

#include "XMatrix22.h"
#include "XMatrix33.h"
#include "XMatrix34.h"
#include "XMatrixAffine2.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////
// Matrix44
template<typename T>
class Matrix44
{
public:
    typedef core::StackString<256, char> Description;

    typedef T TYPE;
    typedef T value_type;
    //
    static const size_t DIM = 4;
    static const size_t DIM_SQ = DIM * DIM;
    static const size_t MEM_LEN = sizeof(T) * DIM_SQ;
    static const T EPSILON;

    //
    // This class is OpenGL friendly and stores the m as how OpenGL would expect it.
    //
    // This data is Colum major.
    //  so coloum 0 is x col 1 is y etc.
    //
    // m[i,j]:
    // | m[0,0] m[0,1] m[0,2] m[0,3] |
    // | m[1,0] m[1,1] m[1,2] m[1,3] |
    // | m[2,0] m[2,1] m[2,2] m[2,3] |
    // | m[3,0] m[3,1] m[3,2] m[3,3] |
    //
    // m[idx]
    // | m[ 0] m[ 4] m[ 8] m[12] |
    // | m[ 1] m[ 5] m[ 9] m[13] |
    // | m[ 2] m[ 6] m[10] m[14] |
    // | m[ 3] m[ 7] m[11] m[15] |
    //
    X_PUSH_WARNING_LEVEL(3)

    union
    {
        T m[DIM_SQ]; // block of cols.
        struct
        {
            // This looks like it's transposed from the above, but it's really not.
            // It just has to be written this way so it follows the right ordering
            // in the memory layout as well as being mathematically correct.

            // m[row][col]
            T m00, m10, m20, m30; // col 0
            T m01, m11, m21, m31; // col 1
            T m02, m12, m22, m32; // col 2
            T m03, m13, m23, m33; // col 3
        };
        // [Cols][Rows]
        T mcols[4][4];
    };
    X_POP_WARNING_LEVEL

    Matrix44();

    explicit Matrix44(T s);

    // OpenGL layout - unless srcIsRowMajor is true
    explicit Matrix44(const T* dt, bool srcIsRowMajor = false);

    // OpenGL layout: m[0]=d0, m[1]=d1, m[2]=d2 ... m[13]=d13, m[14]=d14, m[15]=d15 - unless srcIsRowMajor is true
    Matrix44(T d0, T d1, T d2, T d3, T d4, T d5, T d6, T d7, T d8, T d9, T d10, T d11, T d12, T d13, T d14, T d15, bool srcIsRowMajor = false);

    // Creates matrix with column vectors vx, vy, vz and vw
    Matrix44(const Vec3<T>& vx, const Vec3<T>& vy, const Vec3<T>& vz);
    Matrix44(const Vec4<T>& vx, const Vec4<T>& vy, const Vec4<T>& vz, const Vec4<T>& vw = Vec4<T>(0, 0, 0, 1));

    template<typename FromT>
    explicit Matrix44(const Matrix44<FromT>& src);

    explicit Matrix44(const Matrix22<T>& src);
    explicit Matrix44(const MatrixAffine2<T>& src);
    explicit Matrix44(const Matrix33<T>& src);
    explicit Matrix44(const Matrix34<T>& src);
    Matrix44(const Matrix44<T>& src);


    Matrix44<T>& operator=(const Matrix44<T>& rhs);
    Matrix44<T>& operator=(T rhs);

    template<typename FromT>
    Matrix44<T>& operator=(const Matrix44<FromT>& rhs);

    // remaining columns and rows will be filled with identity values
    Matrix44<T>& operator=(const Matrix22<T>& rhs);
    Matrix44<T>& operator=(const MatrixAffine2<T>& rhs);
    Matrix44<T>& operator=(const Matrix33<T>& rhs);

    operator T*();
    operator const T*() const;

    bool equalCompare(const Matrix44<T>& rhs, T epsilon) const;
    bool operator==(const Matrix44<T>& rhs) const;
    bool operator!=(const Matrix44<T>& rhs) const;

    Matrix44<T>& operator*=(const Matrix44<T>& rhs);
    Matrix44<T>& operator+=(const Matrix44<T>& rhs);
    Matrix44<T>& operator-=(const Matrix44<T>& rhs);

    Matrix44<T>& operator*=(T rhs);
    Matrix44<T>& operator/=(T rhs);
    Matrix44<T>& operator+=(T rhs);
    Matrix44<T>& operator-=(T rhs);

    const Matrix44<T> operator*(const Matrix44<T>& rhs) const;
    const Matrix44<T> operator+(const Matrix44<T>& rhs) const;
    const Matrix44<T> operator-(const Matrix44<T>& rhs) const;

    // post-multiplies column vector [rhs.x rhs.y rhs.z 1] and divides by w
    const Vec3<T> operator*(const Vec3<T>& rhs) const;

    // post-multiplies column vector [rhs.x rhs.y rhs.z rhs.w]
    const Vec4<T> operator*(const Vec4<T>& rhs) const;

    const Matrix44<T> operator*(T rhs) const;
    const Matrix44<T> operator/(T rhs) const;
    const Matrix44<T> operator+(T rhs) const;
    const Matrix44<T> operator-(T rhs) const;

    // Accessors
    T& at(int row, int col);
    const T& at(int row, int col) const;

    // OpenGL layout - unless srcIsRowMajor is true
    void set(const T* dt, bool srcIsRowMajor = false);
    // OpenGL layout: m[0]=d0, m[1]=d1, m[2]=d2 ... m[13]=d13, m[14]=d14, m[15]=d15 - unless srcIsRowMajor is true
    void set(T d0, T d1, T d2, T d3, T d4, T d5, T d6, T d7, T d8, T d9, T d10, T d11, T d12, T d13, T d14, T d15, bool srcIsRowMajor = false);

    Vec4<T> getColumn(int col) const;
    void setColumn(int col, const Vec4<T>& v);

    Vec4<T> getRow(int row) const;
    void setRow(int row, const Vec4<T>& v);

    void getColumns(Vec4<T>* c0, Vec4<T>* c1, Vec4<T>* c2, Vec4<T>* c3) const;
    void setColumns(const Vec4<T>& c0, const Vec4<T>& c1, const Vec4<T>& c2, const Vec4<T>& c3);

    void getRows(Vec4<T>* r0, Vec4<T>* r1, Vec4<T>* r2, Vec4<T>* r3) const;
    void setRows(const Vec4<T>& r0, const Vec4<T>& r1, const Vec4<T>& r2, const Vec4<T>& r3);

    // returns a sub-matrix starting at row, col
    Matrix22<T> subMatrix22(int row, int col) const;
    Matrix33<T> subMatrix33(int row, int col) const;

    void setToNull();
    void setToIdentity();

    T determinant() const;
    // returns trace of 3x3 submatrix if fullTrace == false, otherwise returns trace of full 4x4 matrix
    T trace(bool fullTrace = false) const;

    Matrix44<T> diagonal() const;

    Matrix44<T> lowerTriangular() const;
    Matrix44<T> upperTriangular() const;

    void transpose();
    Matrix44<T> transposed() const;

    void invert(T epsilon = EPSILON);
    Matrix44<T> inverted(T epsilon = EPSILON) const;

    // pre-multiplies row vector v - no divide by w
    Vec3<T> preMultiply(const Vec3<T>& v) const;
    Vec4<T> preMultiply(const Vec4<T>& v) const;

    // post-multiplies column vector v - no divide by w
    Vec3<T> postMultiply(const Vec3<T>& v) const;
    Vec4<T> postMultiply(const Vec4<T>& v) const;

    //! Computes inverse; assumes the matrix is affine, i.e. the bottom row is [0 0 0 1]
    void affineInvert();
    Matrix44<T> affineInverted() const;

    //! Computes inverse; assumes the matrix is orthonormal
    void orthonormalInvert();
    Matrix44<T> orthonormalInverted() const;

    // post-multiplies column vector [rhs.x rhs.y rhs.z 1] and divides by w - same as operator*( const Vec3<T>& )
    Vec3<T> transformPoint(const Vec3<T>& rhs) const;

    // post-multiplies column vector [rhs.x rhs.y rhs.z 1] but omits divide by w
    Vec3<T> transformPointAffine(const Vec3<T>& rhs) const;

    // post-multiplies column vector [rhs.x rhs.y rhs.z 0]
    Vec3<T> transformVec(const Vec3<T>& rhs) const;
    Vec4<T> transformVec(const Vec4<T>& rhs) const;

    // returns the translation values from the last column
    Vec4<T> getTranslate() const;

    // sets the translation values in the last column
    void setTranslate(const Vec3<T>& v);
    void setTranslate(const Vec4<T>& v);

    // multiplies the current matrix by a translation matrix derived from tr
    void translate(const Vec3<T>& tr);
    void translate(const Vec4<T>& tr);

    // multiplies the current matrix by the rotation matrix derived using axis and radians
    void rotate(const Vec3<T>& axis, T radians);
    void rotate(const Vec4<T>& axis, T radians);

    // multiplies the current matrix by the rotation matrix derived using eulerRadians
    void rotate(const Vec3<T>& eulerRadians);
    void rotate(const Vec4<T>& eulerRadians);

    // multiplies the current matrix by the rotation matrix derived using from, to, worldUp
    void rotate(const Vec3<T>& from, const Vec3<T>& to, const Vec3<T>& worldUp);
    void rotate(const Vec4<T>& from, const Vec4<T>& to, const Vec4<T>& worldUp);

    // multiplies the current matrix by the scale matrix derived from supplies parameters
    void scale(T s);
    void scale(const Vec2<T>& v);
    void scale(const Vec3<T>& v);
    void scale(const Vec4<T>& v);

    // transposes rotation sub-matrix and inverts translation
    Matrix44<T> invertTransform() const;

    const char* toString(Description& desc) const;

    // -----------------------------------------------------

    // returns an identity matrix
    static Matrix44<T> identity();
    // returns 1 filled matrix
    static Matrix44<T> one();
    // returns 0 filled matrix
    static Matrix44<T> zero();

    // creates translation matrix
    static Matrix44<T> createTranslation(const Vec3<T>& v, T w = 1);
    static Matrix44<T> createTranslation(const Vec4<T>& v);

    // creates rotation matrix
    static Matrix44<T> createRotation(const Vec3<T>& axis, T radians);
    static Matrix44<T> createRotation(const Vec4<T>& axis, T radians);
    
    static Matrix44<T> createRotation(const Vec3<T>& from, const Vec3<T>& to, const Vec3<T>& worldUp);
    static Matrix44<T> createRotation(const Vec4<T>& from, const Vec4<T>& to, const Vec4<T>& worldUp);
    
    // equivalent to rotate( zAxis, z ), then rotate( yAxis, y ) then rotate( xAxis, x )
    static Matrix44<T> createRotation(const Vec3<T>& eulerRadians);
    static Matrix44<T> createRotation(const Vec4<T>& eulerRadians);
    
    // creates rotation matrix from ortho normal basis (u, v, n)
    static Matrix44<T> createRotationOnb(const Vec3<T>& u, const Vec3<T>& v, const Vec3<T>& w);
    static Matrix44<T> createRotationOnb(const Vec4<T>& u, const Vec4<T>& v, const Vec4<T>& w);

    // creates scale matrix
    static Matrix44<T> createScale(T s);
    static Matrix44<T> createScale(const Vec2<T>& v);
    static Matrix44<T> createScale(const Vec3<T>& v);
    static Matrix44<T> createScale(const Vec4<T>& v);

    // creates a rotation matrix with z-axis aligned to targetDir
    static Matrix44<T> alignZAxisWithTarget(Vec3<T> targetDir, Vec3<T> upDir);
    static Matrix44<T> alignZAxisWithTarget(Vec4<T> targetDir, Vec4<T> upDir);
};

typedef Matrix44<float32_t> Matrix44f;
typedef Matrix44<float64_t> Matrix44d;


template<typename T>
const T Matrix44<T>::EPSILON = math<T>::CMP_EPSILON;

#include "XMatrix44.inl"

#endif // _X_MATH_MATRIX44_H_