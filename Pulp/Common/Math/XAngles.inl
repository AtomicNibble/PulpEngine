

template<typename T>
Angles<T>::Angles() :
    pitch_(0),
    yaw_(0),
    roll_(0)
{
}

template<typename T>
Angles<T>::Angles(T pitch, T yaw, T roll)
{
    pitch_ = pitch;
    yaw_ = yaw;
    roll_ = roll;
}

template<typename T>
Angles<T>::Angles(const Vec3<T>& v)
{
    pitch_ = v.x;
    yaw_ = v.y;
    roll_ = v.z;
}

template<typename T>
Angles<T>::Angles(const Matrix33<T>& mat)
{
    auto col1 = mat.getColumn(0);
    auto col2 = mat.getColumn(1);
    auto col3 = mat.getColumn(2);

    float s = math<T>::sqrt(col1[0] * col1[0] + col1[1] * col1[1]);
    if (s > EPSILON) {
        pitch_ = ::toDegrees(-math<T>::atan2(col1[2], s));
        yaw_ = ::toDegrees(math<T>::atan2(col1[1], col1[0]));
        roll_ = ::toDegrees(math<T>::atan2(col2[2], col3[2]));
    }
    else {
        pitch_ = col1[2] < 0.0f ? 90.0f : -90.0f;
        yaw_ = ::toDegrees(-math<T>::atan2(col2[0], col2[1]));
        roll_ = 0.0f;
    }
}

template<typename T>
void Angles<T>::set(T pitch, T yaw, T roll)
{
    pitch_ = pitch;
    yaw_ = yaw;
    roll_ = roll;
}

template<typename T>
void Angles<T>::setRoll(T roll)
{
    roll_ = roll;
}

template<typename T>
void Angles<T>::setPitch(T pitch)
{
    pitch_ = pitch;
}

template<typename T>
void Angles<T>::setYaw(T yaw)
{
    yaw_ = yaw;
}

template<typename T>
T Angles<T>::roll(void) const
{
    return roll_;
}

template<typename T>
T Angles<T>::pitch(void) const
{
    return pitch_;
}

template<typename T>
T Angles<T>::yaw(void) const
{
    return yaw_;
}

template<typename T>
T Angles<T>::operator[](size_t index) const
{
    X_ASSERT(index < 3, "out of range")(index);
    return (&pitch_)[index];
}

template<typename T>
T& Angles<T>::operator[](size_t index)
{
    X_ASSERT(index < 3, "out of range")(index);
    return (&pitch_)[index];
}

template<typename T>
typename Angles<T>::MyT Angles<T>::operator-() const
{
    return MyT(-pitch_, -yaw_, -roll_);
}

template<typename T>
typename Angles<T>::MyT& Angles<T>::operator=(const MyT& a)
{
    pitch_ = a.pitch_;
    yaw_ = a.yaw_;
    roll_ = a.roll_;
    return *this;
}

template<typename T>
typename Angles<T>::MyT Angles<T>::operator+(const MyT& a) const
{
    return MyT(pitch_ + a.pitch_, yaw_ + a.yaw_, roll_ + a.roll_);
}

template<typename T>
typename Angles<T>::MyT& Angles<T>::operator+=(const MyT& a)
{
    pitch_ += a.pitch_;
    yaw_ += a.yaw_;
    roll_ += a.roll_;
    return *this;
}

template<typename T>
typename Angles<T>::MyT Angles<T>::operator-(const MyT& a) const
{
    return MyT(pitch_ - a.pitch_, yaw_ - a.yaw_, roll_ - a.roll_);
}

template<typename T>
typename Angles<T>::MyT& Angles<T>::operator-=(const MyT& a)
{
    pitch_ -= a.pitch_;
    yaw_ -= a.yaw_;
    roll_ -= a.roll_;
    return *this;
}

template<typename T>
typename Angles<T>::MyT Angles<T>::operator*(const T a) const
{
    return MyT(pitch_ * a, yaw_ * a, roll_ * a);
}

template<typename T>
typename Angles<T>::MyT& Angles<T>::operator*=(const T a)
{
    pitch_ *= a;
    yaw_ *= a;
    roll_ *= a;
    return *this;
}

template<typename T>
typename Angles<T>::MyT Angles<T>::operator/(const T a) const
{
    return MyT(pitch_ / a, yaw_ / a, roll_ / a);
}

template<typename T>
typename Angles<T>::MyT& Angles<T>::operator/=(const T a)
{
    pitch_ /= a;
    yaw_ /= a;
    roll_ /= a;
    return *this;
}

template<typename T>
bool Angles<T>::compare(const MyT& a) const
{
    return pitch_ == a.piatch_ && yaw_ == a.yaw_ && roll_ == a.roll_;
}

template<typename T>
bool Angles<T>::compare(const MyT& a, const T epsilon) const
{
    return math<T>::abs(pitch_ - a.pitch_) <= epsilon && math<T>::abs(yaw_ - a.yaw_) <= epsilon && math<T>::abs(roll_ - a.roll_) <= epsilon;
}

template<typename T>
bool Angles<T>::operator==(const MyT& a) const
{
    return compare(a);
}

template<typename T>
bool Angles<T>::operator!=(const MyT& a) const
{
    return !compare(a);
}

template<typename T>
typename Angles<T>::MyT& Angles<T>::normalize360(void)
{
    for (int32_t i = 0; i < 3; i++) {
        if (((*this)[i] >= 360.0f) || ((*this)[i] < 0.0f)) {
            (*this)[i] -= floor((*this)[i] / 360.0f) * 360.0f;

            if ((*this)[i] >= 360.0f) {
                (*this)[i] -= 360.0f;
            }
            if ((*this)[i] < 0.0f) {
                (*this)[i] += 360.0f;
            }
        }
    }

    return *this;
}

template<typename T>
typename Angles<T>::MyT& Angles<T>::normalize180(void)
{
    normalize360();

    if (pitch_ > 180.0f) {
        pitch_ -= 360.0f;
    }

    if (yaw_ > 180.0f) {
        yaw_ -= 360.0f;
    }

    if (roll_ > 180.0f) {
        roll_ -= 360.0f;
    }
    return *this;
}

template<typename T>
void Angles<T>::clamp(const MyT& min, const MyT& max)
{
    if (pitch_ < min.pitch_) {
        pitch_ = min.pitch_;
    }
    else if (pitch_ > max.pitch_) {
        pitch_ = max.pitch_;
    }

    if (yaw_ < min.yaw_) {
        yaw_ = min.yaw_;
    }
    else if (yaw_ > max.yaw_) {
        yaw_ = max.yaw_;
    }

    if (roll_ < min.roll_) {
        roll_ = min.roll_;
    }
    else if (roll_ > max.roll_) {
        roll_ = max.roll_;
    }
}

template<typename T>
Vec3<T> Angles<T>::toVec3(void) const
{
    return Vec3<T>(pitch_, yaw_, roll_);
}

template<typename T>
Vec3<T> Angles<T>::toVec3Radians(void) const
{
    return Vec3<T>(::toRadians(pitch_), ::toRadians(yaw_), ::toRadians(roll_));
}

template<typename T>
Vec3<T> Angles<T>::toForward(void) const
{
    float sp, sy, cp, cy;

    math<T>::sincos(::toRadians(yaw_), sy, cy);
    math<T>::sincos(::toRadians(pitch_), sp, cp);

    return Vec3<T>(cp * cy, cp * sy, -sp);
}

template<typename T>
Quat<T> Angles<T>::toQuat(void) const
{
    T sx, cx, sy, cy, sz, cz;
    T sxcy, cxcy, sxsy, cxsy;

    math<T>::sincos(::toRadians(yaw_) * T(0.5f), sz, cz);
    math<T>::sincos(::toRadians(pitch_) * T(0.5f), sy, cy);
    math<T>::sincos(::toRadians(roll_) * T(0.5f), sx, cx);

    sxcy = sx * cy;
    cxcy = cx * cy;
    sxsy = sx * sy;
    cxsy = cx * sy;

    return Quat<T>(
        cxcy * cz + sxsy * sz,
        cxsy * sz - sxcy * cz,
        -cxsy * cz - sxcy * sz,
        sxsy * cz - cxcy * sz);
}

template<typename T>
Matrix33<T> Angles<T>::toMat3(void) const
{
    T sr, sp, sy, cr, cp, cy;

    math<T>::sincos(::toRadians(yaw_), sy, cy);
    math<T>::sincos(::toRadians(roll_), sp, cp);
    math<T>::sincos(::toRadians(pitch_), sr, cr);

    Matrix33<T> mat(
        cp * cy, cp * sy, -sp,
        sr * sp * cy + cr * -sy, sr * sp * sy + cr * cy, sr * cp,
        cr * sp * cy + -sr * -sy, cr * sp * sy + -sr * cy, cr * cp);

    return mat;
}

template<typename T>
Matrix44<T> Angles<T>::toMat4(void) const
{
    return Matrix44<T>(toMat3());
}

template<typename T>
Vec3<T> Angles<T>::toAngularVelocity(void) const
{
}

template<typename T>
const char* Angles<T>::toString(StrBuf& buf) const
{
    buf.setFmt("<%g,%g,%g>", pitch_, yaw_, roll_);
    return buf.c_str();
}

template<typename T>
typename Angles<T>::MyT Angles<T>::zero(void)
{
    return MyT(0.f, 0.f, 0.f);
}