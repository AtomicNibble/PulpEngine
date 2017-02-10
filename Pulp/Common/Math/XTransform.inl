

template<typename T>
X_INLINE Transform<T>::Transform()
{

}

template<typename T>
X_INLINE Transform<T>::Transform(const Vec3<T>& vec_, const Quat<T>& quat_) :
	quat(quat_),
	trans(vec_)
{
}

template<typename T>
template<typename TOth>
X_INLINE Transform<T>::Transform(const Transform<TOth>& oth)
{
	quat = oth.quat;
	trans = oth.trans;
}

template<typename T>
X_INLINE Transform<T>::Transform(const Matrix34<T>& mat)
{
	set(mat);
}

template<typename T>
X_INLINE Transform<T>& Transform<T>::operator=(const Transform<T>& qt)
{
	quat = qt.quat;
	trans = qt.trans;
	return *this;
}

template<typename T>
X_INLINE bool Transform<T>::operator==(const Transform<T> &rhs) const
{
	return quat = rhs.quat && trans == rhs.trans;
}

template<typename T>
X_INLINE bool Transform<T>::operator!=(const Transform<T> &rhs) const
{
	return !(*this == rhs);
}

template<typename T>
X_INLINE void Transform<T>::set(const Vec3<T> &trans_, const Quat<T>& qt_)
{
	trans = trans_;
	quat = qt_;
}

template<typename T>
X_INLINE void Transform<T>::set(const Matrix34<T>& mat)
{
	quat = Quat<T>(mat);
	trans = mat.getTranslate();
}

template<typename T>
void Transform<T>::setTranslation(const Vec3<T>& vec)
{
	trans = vec;
}

template<typename T>
Vec3<T> Transform<T>::getTranslation(void) const
{
	return trans;
}
