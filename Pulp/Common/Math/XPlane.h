#pragma once

#ifndef _X_MATH_PLANE_H_
#define _X_MATH_PLANE_H_

#include "XVector.h"

//
//
//	+
//	|\
//  | \
//  |  \
//  |  /\
//	| /  \
//  |/    \
//  +------+
//
//	A plane 3 points in 3d space.
//  we need 3 points so that the orientation(pitch, yaw, roll)
//  can be determinied.
//
// We store it as a normal and ditance as describied in Methord 3
// http://en.wikipedia.org/wiki/Plane_(geometry)
//
//

X_DECLARE_ENUM(PlaneSide)
(FRONT, BACK, ON, CROSS);

struct PlaneType
{
    enum Enum
    {
        X,
        Y,
        Z,
        NEGX,
        NEGY,
        NEGZ,
        ZEROX,
        ZEROY,
        ZEROZ,
        NONAXIAL
    };

    static X_INLINE bool isTrueAxial(Enum type)
    {
        return type < ZEROX;
    }
};

template<typename T>
class Plane
{
public:
    typedef core::StackString<128, char> Description;

public:
    X_INLINE Plane();
    X_INLINE Plane(const Vec3<T>& v1, const Vec3<T>& v2, const Vec3<T>& v3);
    X_INLINE Plane(const Vec3<T>& point, const Vec3<T>& normal);
    X_INLINE Plane(const Vec3<T>& normal, const T distance);
    X_INLINE Plane(T a, T b, T c, T d);

    //! Defines a plane using 3 points.
    X_INLINE void set(const Vec3<T>& v1, const Vec3<T>& v2, const Vec3<T>& v3);
    //! Defines a plane using a normal vector and a point.
    X_INLINE void set(const Vec3<T>& point, const Vec3<T>& normal);
    //! Defines a plane using a normal and distance
    X_INLINE void set(const Vec3<T>& normal, const T distance);
    //! Defines a plane using 4 coefficients.
    X_INLINE void set(T a, T b, T c, T d);

    X_INLINE T operator[](size_t idx) const;
    X_INLINE T& operator[](size_t idx);

    X_INLINE Plane operator-() const;

    X_INLINE Vec3<T> getPoint(void) const;
    X_INLINE const Vec3<T>& getNormal(void) const;
    X_INLINE void setNormal(const Vec3<T>& normal);
    X_INLINE T getDistance(void) const;
    X_INLINE void setDistance(const T distance);
    X_INLINE T distance(const Vec3<T>& p) const;

    X_INLINE Vec3<T> reflectVector(const Vec3<T>& v) const;
    X_INLINE Vec3<T> reflectPoint(const Vec3<T>& p) const;

    X_INLINE bool rayIntersection(const Ray& ray, Vec3f& out);
    X_INLINE bool compare(const Plane& p, const T epsilon) const;
    X_INLINE bool compare(const Plane& p, const T normalEps, const T distEps) const;

    X_INLINE T dot(Plane<T>& oth) const;

    X_INLINE PlaneSide::Enum side(const Vec3<T>& v, const T epsilon) const;
    X_INLINE PlaneType::Enum getType(void) const;
    X_INLINE bool isTrueAxial(void) const;

    X_INLINE const char* toString(Description& desc) const;

private:
    Vec3<T> normal_;
    T distance_;
};

#include "XPlane.inl"

typedef Plane<float> Planef;
typedef Plane<double> Planed;

#endif // !_X_MATH_PLANE_H_
