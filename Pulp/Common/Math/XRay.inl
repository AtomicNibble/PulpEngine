
X_INLINE Ray::Ray(const Vec3f& aOrigin, const Vec3f& aDirection) :
    Origin_(aOrigin)
{
    setDirection(aDirection);
}

X_INLINE void Ray::setOrigin(const Vec3f& aOrigin)
{
    Origin_ = aOrigin;
}

X_INLINE const Vec3f& Ray::getOrigin(void) const
{
    return Origin_;
}

X_INLINE void Ray::setDirection(const Vec3f& aDirection)
{
    Direction_ = aDirection;
}

X_INLINE const Vec3f& Ray::getDirection(void) const
{
    return Direction_;
}

X_INLINE Vec3f Ray::calcPosition(float distance) const
{
    return Origin_ + Direction_ * distance;
}
