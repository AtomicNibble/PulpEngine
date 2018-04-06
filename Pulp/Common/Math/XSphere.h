#pragma once

#ifndef _X_MATH_SPHERE_H_
#define _X_MATH_SPHERE_H_

#include "XVector.h"
#include "XRay.h"

// #include "XAabb.h"
class AABB;

class Sphere
{
public:
    X_INLINE Sphere() = default;
    X_INLINE Sphere(const Vec3f& aCenter, float aRadius);
    explicit Sphere(const AABB& box);

    X_INLINE float radius(void) const;
    X_INLINE void setRadius(float radius);

    X_INLINE const Vec3f& center(void) const;
    X_INLINE const void setCenter(const Vec3f& center);

    X_INLINE bool intersects(const Ray& ray);
    X_INLINE bool intersect(const Ray& ray, float* intersection);

protected:
    Vec3f center_;
    float radius_;
};

#include "XSphere.inl"

#endif // !_X_MATH_SPHERE_H_
