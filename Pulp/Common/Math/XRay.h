#pragma once

#ifndef _X_MATH_RAY_H_
#define _X_MATH_RAY_H_

#include "XVector.h"

class Ray
{
public:
    Ray()
    {
    }
    Ray(const Vec3f& aOrigin, const Vec3f& aDirection) :
        Origin_(aOrigin)
    {
        setDirection(aDirection);
    }

    X_INLINE void setOrigin(const Vec3f& aOrigin)
    {
        Origin_ = aOrigin;
    }
    X_INLINE const Vec3f& getOrigin() const
    {
        return Origin_;
    }

    X_INLINE void setDirection(const Vec3f& aDirection)
    {
        Direction_ = aDirection;
    }
    X_INLINE const Vec3f& getDirection() const
    {
        return Direction_;
    }

    X_INLINE Vec3f calcPosition(float t) const
    {
        return Origin_ + Direction_ * t;
    }

protected:
    Vec3f Origin_;
    Vec3f Direction_;
};

#endif // !_X_MATH_RAY_H_
