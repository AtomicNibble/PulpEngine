#pragma once

#ifndef _X_MATH_QUAT_H_
#define _X_MATH_QUAT_H_

#include "XMath.h"
#include "XVector.h"

#include "XMatrix33.h"
#include "XMatrix34.h"
#include "XMatrix44.h"


template<typename T>
class Quat
{
public:
    typedef T TYPE;
    typedef T value_type;

    static const size_t DIM = 4;
    static const size_t MEM_LEN = sizeof(T) * DIM;
    static const T EPSILON;

    Vec3<T> v; // axisOfRotation.normalized() * sin( angleOfRotation / 2 )
    T w;       // cos( angleOfRotation / 2 )

    Quat();
    
    template<typename FromT>
    explicit Quat(const Quat<FromT>& q);

    Quat(T aW, T x, T y, T z);
    
    Quat(T _w, const Vec3<T>& vec);                 // construct from axis-angle
    Quat(const Vec3<T>& axis, T radians);
    Quat(const Vec3<T>& from, const Vec3<T>& to);
    Quat(const Vec3<T>& eulerRadians);              // create from Euler angles in radians expressed in ZYX rotation order
    Quat(T pitch, T yaw, T roll);                   // create from Euler angles in radians expressed in ZYX rotation order
    explicit Quat(const Matrix33<T>& m);
    explicit Quat(const Matrix44<T>& m);
    explicit Quat(const Matrix34<T>& m);

    // Operators
    Quat<T>& operator=(const Quat<T>& rhs);
    template<typename FromT>
    Quat<T>& operator=(const Quat<FromT>& rhs);

    // post-multiply operator, similar to matrices, but different from Shoemake
    // Concatenates 'rhs' onto 'this'
    const Quat<T> operator*(const Quat<T>& rhs) const;
    const Quat<T> operator*(T rhs) const;
    const Vec3<T> operator*(const Vec3<T>& vec) const; // // transform a vector by the Quat
    const Quat<T> operator+(const Quat<T>& rhs) const;
    const Quat<T> operator-(const Quat<T>& rhs) const;
    Quat<T>& operator+=(const Quat<T>& rhs);
    Quat<T>& operator-=(const Quat<T>& rhs);
    Quat<T>& operator*=(const Quat<T>& rhs);
    Quat<T>& operator*=(T rhs);

    Quat<T> operator~() const;
    Quat<T> operator-() const;

    bool operator==(const Quat<T>& rhs) const;
    bool operator!=(const Quat<T>& rhs) const;
    bool compare(const Quat<T>& oth, const T elipson = EPSILON) const;

    T& operator[](uint32_t i);
    const T& operator[](uint32_t i) const;

    void set(T aW, T x, T y, T z);
    void set(const Vec3<T>& from, const Vec3<T>& to);
    void set(const Vec3<T>& axis, T radians);
    void set(const Vec3<T>& eulerRadians);
    void set(T pitch, T yaw, T roll);
    void set(const Matrix33<T>& m);
    void set(const Matrix34<T>& m);
    void set(const Matrix44<T>& m);

    void getAxisAngle(Vec3<T>* axis, T* radians) const;

    Vec3<T> getAxis() const;    // get axis-angle representation's axis
    T getAngle() const;         // get axis-angle representation's angle in radians
    T getPitch() const;
    T getYaw() const;
    T getRoll() const;
    T dot(const Quat<T>& quat) const;
    T length() const;
    T lengthSquared() const;
    Vec3<T> getEuler(void) const;
    Vec3<T> getEulerDegrees(void) const;

    Quat<T>& normalize();
    Quat<T> normalized() const;
    Quat<T> log() const;
    Quat<T> exp() const;
    Quat<T> inverse() const;
    Quat<T> inverted() const;
    Quat<T>& invert();
    Quat<T> diff(const Quat& oth) const;

    Matrix33<T> toMatrix33() const;
    Matrix44<T> toMatrix44() const;

    // fancy things.
    Quat<T> lerp(T t, const Quat<T>& end) const;
    Quat<T> slerpShortestUnenforced(T t, const Quat<T>& end) const;
    Quat<T> slerp(T t, const Quat<T>& end) const;
    Quat<T> squadShortestEnforced(T t, const Quat<T>& qa, const Quat<T>& qb, const Quat<T>& q2) const;
    Quat<T> squad(T t, const Quat<T>& qa, const Quat<T>& qb, const Quat<T>& q2) const;
    Quat<T> spline(T t, const Quat<T>& q1, const Quat<T>& q2, const Quat<T>& q3) const;

    static Quat<T> identity();

private:
    static Quat<T> splineIntermediate(const Quat<T>& q0, const Quat<T>& q1, const Quat<T>& q2);
};

typedef Quat<float> Quatf;
typedef Quat<double> Quatd;

#include "XQuat.inl"

template<typename T>
const T Quat<T>::EPSILON = math<T>::CMP_EPSILON;

template<typename T>
inline Vec3<T> operator*(const Vec3<T>& vec, const Quat<T>& q)
{
    T vMult = T(2) * (q.v.x * vec.x + q.v.y * vec.y + q.v.z * vec.z);
    T crossMult = T(2) * q.w;
    T pMult = crossMult * q.w - T(1);

    return Vec3<T>(pMult * vec.x + vMult * q.v.x + crossMult * (q.v.y * vec.z - q.v.z * vec.y),
        pMult * vec.y + vMult * q.v.y + crossMult * (q.v.z * vec.x - q.v.x * vec.z),
        pMult * vec.z + vMult * q.v.z + crossMult * (q.v.x * vec.y - q.v.y * vec.x));
}


#endif // !_X_MATH_QUAT_H_
