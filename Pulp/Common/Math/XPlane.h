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
    X_INLINE Plane(const Vec3<T>& normal, const T disatnace);
    X_INLINE Plane(T a, T b, T c, T d);

    //! Defines a plane using 3 points.
    X_INLINE void set(const Vec3<T>& v1, const Vec3<T>& v2, const Vec3<T>& v3);
    //! Defines a plane using a normal vector and a point.
    X_INLINE void set(const Vec3<T>& point, const Vec3<T>& normal);
    //! Defines a plane using a normal and distance
    X_INLINE void set(const Vec3<T>& normal, const T disatnace);
    //! Defines a plane using 4 coefficients.
    X_INLINE void set(T a, T b, T c, T d);

    X_INLINE T operator[](size_t idx) const;
    X_INLINE T& operator[](size_t idx);

    X_INLINE Plane operator-() const;

    X_INLINE Vec3<T> getPoint(void) const;
    X_INLINE const Vec3<T>& getNormal(void) const;
    X_INLINE void setNormal(const Vec3<T>& normal);
    X_INLINE T getDistance(void) const;
    X_INLINE void setDistance(const float distance);
    X_INLINE T distance(const Vec3<T>& p) const;

    X_INLINE Vec3<T> reflectVector(const Vec3<T>& v) const;
    X_INLINE Vec3<T> reflectPoint(const Vec3<T>& p) const;

    X_INLINE bool rayIntersection(const Ray& ray, Vec3f& out);
    X_INLINE bool compare(const Plane& p, const float normalEps, const float distEps) const;

    X_INLINE T dot(Plane<T>& oth) const;

    X_INLINE PlaneSide::Enum side(const Vec3<T>& v, const float epsilon) const;
    X_INLINE PlaneType::Enum getType(void) const;
    X_INLINE bool isTrueAxial(void) const;

    X_INLINE const char* toString(Description& desc) const;

private:
    Vec3<T> normal_;
    T distance_;
};

template<typename T>
X_INLINE Plane<T>::Plane() :
    distance_(static_cast<T>(0))
{
}

template<typename T>
X_INLINE Plane<T>::Plane(const Vec3<T>& v1, const Vec3<T>& v2, const Vec3<T>& v3)
{
    set(v1, v2, v3);
}

template<typename T>
X_INLINE Plane<T>::Plane(const Vec3<T>& point, const Vec3<T>& normal)
{
    set(point, normal);
}

template<typename T>
X_INLINE Plane<T>::Plane(const Vec3<T>& normal, const T disatnace)
{
    set(normal, disatnace);
}

template<typename T>
X_INLINE Plane<T>::Plane(T a, T b, T c, T d)
{
    set(a, b, c, d);
}

template<typename T>
X_INLINE void Plane<T>::set(const Vec3<T>& v1, const Vec3<T>& v2, const Vec3<T>& v3)
{
    normal_ = (v1 - v2).cross(v3 - v2);
    normal_.normalizeSafe();
    distance_ = -(normal_ * v2);
}

template<typename T>
X_INLINE void Plane<T>::set(const Vec3<T>& point, const Vec3<T>& normal)
{
    normal_ = normal.normalized();
    distance_ = -(normal_.dot(point));
}

template<typename T>
X_INLINE void Plane<T>::set(const Vec3<T>& normal, const T disatnace)
{
    normal_ = normal.normalized();
    distance_ = disatnace;
}

template<typename T>
X_INLINE void Plane<T>::set(T a, T b, T c, T d)
{
    Vec3<T> normal(a, b, c);
    T length = normal.length();

    normal_ = normal.normalized();
    distance_ = d / length;
}

template<typename T>
X_INLINE T Plane<T>::operator[](size_t idx) const
{
    return normal_[idx];
}

template<typename T>
X_INLINE T& Plane<T>::operator[](size_t idx)
{
    return normal_[idx];
}

template<typename T>
X_INLINE Plane<T> Plane<T>::operator-() const
{
    return Plane(-normal_, -distance_);
}

template<typename T>
X_INLINE Vec3<T> Plane<T>::getPoint(void) const
{
    return normal_ * getDistance();
}

template<typename T>
X_INLINE const Vec3<T>& Plane<T>::getNormal(void) const
{
    return normal_;
}

template<typename T>
X_INLINE void Plane<T>::setNormal(const Vec3<T>& normal)
{
    normal_ = normal;
}

template<typename T>
X_INLINE T Plane<T>::getDistance(void) const
{
    return -distance_;
}

template<typename T>
X_INLINE void Plane<T>::setDistance(const float distance)
{
    distance_ = -distance;
}

template<typename T>
X_INLINE T Plane<T>::distance(const Vec3<T>& p) const
{
    return normal_.dot(p) + distance_;
};

template<typename T>
X_INLINE Vec3<T> Plane<T>::reflectPoint(const Vec3<T>& p) const
{
    return normal_ * distance(p) * -2 + p;
}

template<typename T>
X_INLINE Vec3<T> Plane<T>::reflectVector(const Vec3<T>& v) const
{
    return normal_ * normal_.dot(v) * 2 - v;
}

template<typename T>
X_INLINE bool Plane<T>::rayIntersection(const Ray& ray, Vec3f& out)
{
    float cosine = normal_.dot(ray.getDirection());

    if (cosine == 0.f) // parallel.
        return false;

    float dis = distance(ray.getOrigin());
    float scale = -(dis / cosine);

    out = ray.getOrigin() + (ray.getDirection() * scale);
    return true;
}

template<typename T>
X_INLINE bool Plane<T>::compare(const Plane& p, const float normalEps, const float distEps) const
{
    if (math<float>::abs(distance_ - p.distance_) > distEps) {
        return false;
    }
    if (!normal_.compare(p.getNormal(), normalEps)) {
        return false;
    }
    return true;
}

template<typename T>
X_INLINE T Plane<T>::dot(Plane<T>& oth) const
{
    return normal_.dot(oth.normal_);
}

template<typename T>
X_INLINE PlaneSide::Enum Plane<T>::side(const Vec3<T>& v, const float epsilon) const
{
    float dist = distance(v);
    if (dist > epsilon) {
        return PlaneSide::FRONT;
    }
    else if (dist < -epsilon) {
        return PlaneSide::BACK;
    }

    return PlaneSide::ON;
}

template<typename T>
X_INLINE PlaneType::Enum Plane<T>::getType(void) const
{
    if (normal_[0] == 0.0f) {
        if (normal_[1] == 0.0f) {
            return normal_[2] > 0.0f ? PlaneType::Z : PlaneType::NEGZ;
        }
        else if (normal_[2] == 0.0f) {
            return normal_[1] > 0.0f ? PlaneType::Y : PlaneType::NEGY;
        }
        else {
            return PlaneType::ZEROX;
        }
    }
    else if (normal_[1] == 0.0f) {
        if (normal_[2] == 0.0f) {
            return normal_[0] > 0.0f ? PlaneType::X : PlaneType::NEGX;
        }
        else {
            return PlaneType::ZEROY;
        }
    }
    else if (normal_[2] == 0.0f) {
        return PlaneType::ZEROZ;
    }

    return PlaneType::NONAXIAL;
}

template<typename T>
X_INLINE bool Plane<T>::isTrueAxial(void) const
{
    return getType() < PlaneType::ZEROX;
}

template<typename T>
X_INLINE const char* Plane<T>::toString(Description& desc) const
{
    desc.setFmt("<%g,%g,%g> - %g", normal_.x, normal_.y, normal_.z, distance_);
    return desc.c_str();
}

typedef Plane<float> Planef;
typedef Plane<double> Planed;

#endif // !_X_MATH_PLANE_H_
