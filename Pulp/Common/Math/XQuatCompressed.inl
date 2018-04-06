
template<typename T>
XQuatCompressed<T>::XQuatCompressed() :
    w_(0)
{
}

template<typename T>
XQuatCompressed<T>::XQuatCompressed(const Quat<T>& q)
{
    set(q);
}

template<typename T>
XQuatCompressed<T>::XQuatCompressed(const Matrix33<T>& m)
{
    set(m);
}

template<typename T>
XQuatCompressed<T>::XQuatCompressed(T aW, T x, T y, T z)
{
    set(aW, x, y, z);
}

// -----------------------------------------------

template<typename T>
void XQuatCompressed<T>::set(const Quat<T>& q_)
{
    // need to compress it as a float.
    Quat<T> q(q_);

    T scale = math<T>::sqrt(q.v.x * q.v.x + q.v.y * q.v.y + q.v.z * q.v.z + q.w * q.w);

    scale = math<T>::floor(scale * (((T)32767.0) / scale));

    q *= scale;

    v_.set(static_cast<comp_type>(q.v.x), static_cast<comp_type>(q.v.y), static_cast<comp_type>(q.v.z));
    w_ = static_cast<comp_type>(q.w);
}

template<typename T>
void XQuatCompressed<T>::set(const Matrix33<T>& m)
{
    set(Quat<T>(m));
}

template<typename T>
void XQuatCompressed<T>::set(T aW, T x, T y, T z)
{
    set(Quat<T>(aW, x, y, z));
}

// -----------------------------------------------

template<class T>
Quat<T> XQuatCompressed<T>::asQuat(void) const
{
    Quat<T> q;
    q.set(w_, v_.x, v_.y, v_.z);

    q.v /= 32766.5;
    q.w /= 32766.5;

    return q;
}

template<class T>
Matrix33<T> XQuatCompressed<T>::asMatrix33(void) const
{
    return asQuat().toMatrix33();
}

template<typename T>
XQuatCompressed<T>& XQuatCompressed<T>::operator=(const XQuatCompressed<T>& rhs)
{
    v_ = rhs.v_;
    w_ = rhs.w_;
    return *this;
}

template<typename T>
bool XQuatCompressed<T>::operator==(const XQuatCompressed<T>& rhs) const
{
    return v_ = rhs.v_ && w_ == rhs.w_;
}

template<typename T>
bool XQuatCompressed<T>::operator!=(const XQuatCompressed<T>& rhs) const
{
    return !(*this == rhs);
}

template<typename T>
T& XQuatCompressed<T>::operator[](size_t i)
{
    return (&v_.x)[i];
}

template<typename T>
const T& XQuatCompressed<T>::operator[](size_t i) const
{
    return (&v_.x)[i];
}

template<typename T>
XQuatCompressed<T> XQuatCompressed<T>::identity(void)
{
    return XQuatCompressed();
}
