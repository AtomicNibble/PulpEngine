

template<typename T>
X_INLINE QuatTrans<T>::QuatTrans()
{

}

template<typename T>
X_INLINE QuatTrans<T>::QuatTrans(const Vec3<T>& vec_, const Quat<T>& quat_) :
	quat(quat_),
	trans(vec_)
{
}

template<typename T>
template<typename TOth>
X_INLINE QuatTrans<T>::QuatTrans(const QuatTrans<TOth>& oth)
{
	quat = oth.quat;
	trans = oth.trans;
}

template<typename T>
X_INLINE QuatTrans<T>::QuatTrans(const Matrix34<T>& mat)
{
	set(mat);
}

template<typename T>
X_INLINE QuatTrans<T>& QuatTrans<T>::operator=(const QuatTrans<T>& qt)
{
	quat = qt.quat;
	trans = qt.trans;
	return *this;
}

template<typename T>
X_INLINE bool QuatTrans<T>::operator==(const QuatTrans<T> &rhs) const
{
	return quat = rhs.quat && trans == rhs.trans;
}

template<typename T>
X_INLINE bool QuatTrans<T>::operator!=(const QuatTrans<T> &rhs) const
{
	return !(*this == rhs);
}

template<typename T>
X_INLINE void QuatTrans<T>::set(const Vec3<T> &trans_, const Quat<T>& qt_)
{
	trans = trans_;
	quat = qt_;
}

template<typename T>
X_INLINE void QuatTrans<T>::set(const Matrix34<T>& mat)
{
	quat = Quat<T>(mat);
	trans = mat.getTranslate();
}

template<typename T>
void QuatTrans<T>::setTranslation(const Vec3<T>& vec)
{
	trans = vec;
}

template<typename T>
Vec3<T> QuatTrans<T>::getTranslation(void) const
{
	return trans;
}
