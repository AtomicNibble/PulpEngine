
X_INLINE Sphere::Sphere(const Vec3f& aCenter, float aRadius) :
    center_(aCenter),
    radius_(aRadius)
{
}

X_INLINE float Sphere::radius(void) const
{
    return radius_;
}

X_INLINE void Sphere::setRadius(float radius)
{
    radius_ = radius;
}

X_INLINE const Vec3f& Sphere::center(void) const
{
    return center_;
}

X_INLINE const void Sphere::setCenter(const Vec3f& center)
{
    center_ = center;
}

X_INLINE bool Sphere::intersects(const Ray& ray)
{
    float t;
    Vec3f temp = ray.getOrigin() - center_;
    float a = ray.getDirection().dot(ray.getDirection());
    float b = 2.0f * temp.dot(ray.getDirection());
    float c = temp.dot(temp) - radius_ * radius_;
    float disc = b * b - 4.0f * a * c;

    if (disc < 0.0f) {
        return false;
    }
    else { // this probably can be optimized
        float e = math<float>::sqrt(disc);
        float denom = 2.0f * a;
        t = (-b - e) / denom; // smaller root

        if (t > EPSILON_VALUE) {
            return true;
        }

        t = (-b + e) / denom; // larger root
        if (t > EPSILON_VALUE) {
            return true;
        }
    }

    return false;
}

X_INLINE bool Sphere::intersect(const Ray& ray, float* intersection)
{
    float t;
    Vec3f temp = ray.getOrigin() - center_;
    float a = ray.getDirection().dot(ray.getDirection());
    float b = 2.0f * temp.dot(ray.getDirection());
    float c = temp.dot(temp) - radius_ * radius_;
    float disc = b * b - 4.0f * a * c;

    if (disc < 0.0f) {
        return false;
    }
    else {
        float e = math<float>::sqrt(disc);
        float denom = 2.0f * a;
        t = (-b - e) / denom; // smaller root

        if (t > EPSILON_VALUE) {
            *intersection = t;
            return true;
        }

        t = (-b + e) / denom; // larger root
        if (t > EPSILON_VALUE) {
            *intersection = t;
            return true;
        }
    }

    return false;
}