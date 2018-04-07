#pragma once

#ifndef _X_MATH_RAY_H_
#define _X_MATH_RAY_H_

#include "XVector.h"

class Ray
{
public:
    Ray() = default;

    X_INLINE Ray(const Vec3f& aOrigin, const Vec3f& aDirection);

    X_INLINE void setOrigin(const Vec3f& aOrigin);
    X_INLINE const Vec3f& getOrigin(void) const;
    X_INLINE void setDirection(const Vec3f& aDirection);
    X_INLINE const Vec3f& getDirection(void) const;
    X_INLINE Vec3f calcPosition(float t) const;

protected:
    Vec3f Origin_;
    Vec3f Direction_;
};

#include "XRay.inl"

#endif // !_X_MATH_RAY_H_
