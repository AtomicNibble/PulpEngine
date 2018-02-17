

template<typename T>
X_INLINE Transform<T>::Transform()
{

}

template<typename T>
X_INLINE Transform<T>::Transform(const Vec3<T>& vec_, const Quat<T>& quat_) :
	quat(quat_),
	pos(vec_)
{
}

template<typename T>
template<typename TOth>
X_INLINE Transform<T>::Transform(const Transform<TOth>& oth)
{
	quat = oth.quat;
	pos = oth.pos;
}

template<typename T>
X_INLINE Transform<T>::Transform(const Matrix34<T>& mat)
{
	set(mat);
}

template<typename T>
X_INLINE Transform<T>::Transform(const Matrix44<T>& mat)
{
	set(mat);
}

template<typename T>
X_INLINE Transform<T>& Transform<T>::operator=(const Transform<T>& qt)
{
	quat = qt.quat;
	pos = qt.pos;
	return *this;
}

template<typename T>
X_INLINE bool Transform<T>::operator==(const Transform<T> &rhs) const
{
	return quat == rhs.quat && pos == rhs.pos;
}

template<typename T>
X_INLINE bool Transform<T>::operator!=(const Transform<T> &rhs) const
{
	return !(*this == rhs);
}

template<typename T>
X_INLINE void Transform<T>::set(const Vec3<T> &pos_, const Quat<T>& qt_)
{
	pos = pos_;
	quat = qt_;
}

template<typename T>
X_INLINE void Transform<T>::set(const Matrix34<T>& mat)
{
	quat = Quat<T>(mat);
	pos = mat.getTranslate();
}

template<typename T>
X_INLINE void Transform<T>::set(const Matrix44<T>& mat)
{
	quat = Quat<T>(mat);
	pos = mat.getTranslate().xyz();
}

template<typename T>
X_INLINE void Transform<T>::setPosition(const Vec3<T>& vec)
{
	pos = vec;
}


template<typename T>
X_INLINE Vec3<T> Transform<T>::getPosition(void)
{
	return pos;
}

template<typename T>
X_INLINE const Vec3<T>& Transform<T>::getPosition(void) const
{
	return pos;
}

template<typename T>
Vec3<T> Transform<T>::transform(const Vec3<T>& p) const
{
	return (p * quat) + pos;
}
