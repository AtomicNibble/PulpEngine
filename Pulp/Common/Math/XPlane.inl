


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
X_INLINE Plane<T>::Plane(const Vec3<T>& normal, const T distance)
{
    set(normal, distance);
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
X_INLINE void Plane<T>::set(const Vec3<T>& normal, const T distance)
{
    normal_ = normal.normalized();
    distance_ = -distance;
}

template<typename T>
X_INLINE void Plane<T>::set(T a, T b, T c, T d)
{
    Vec3<T> normal(a, b, c);
    T length = normal.length();

    normal_ = normal.normalized();
    distance_ = -(d / length);
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
X_INLINE void Plane<T>::setDistance(const T distance)
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
    T cosine = normal_.dot(ray.getDirection());

    if (cosine == 0.f) { // parallel.
        return false;
    }

    T dis = distance(ray.getOrigin());
    T scale = -(dis / cosine);

    out = ray.getOrigin() + (ray.getDirection() * scale);
    return true;
}

template<typename T>
X_INLINE bool Plane<T>::compare(const Plane& p, const T epsilon) const
{
    return compare(p, epsilon);
}

template<typename T>
X_INLINE bool Plane<T>::compare(const Plane& p, const T normalEps, const T distEps) const
{
    if (math<T>::abs(distance_ - p.distance_) > distEps) {
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
X_INLINE PlaneSide::Enum Plane<T>::side(const Vec3<T>& v, const T epsilon) const
{
    T dist = distance(v);
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
