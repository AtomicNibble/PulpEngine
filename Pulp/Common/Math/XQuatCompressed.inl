

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
	set(qW,x,y,z);
}

// -----------------------------------------------

template<typename T>
void XQuatCompressed<T>::set(const Quat<T>& q_)
{
	// need to compress it as a float.
	Quat<T> q(q_);

	T scale = math<T>::sqrt(q.v.x * q.v.x +
		q.v.y * q.v.y +
		q.v.z * q.v.z +
		q.w	  * q.w);

	scale = math<T>::floor(scale * (((T)32767.0) / scale));

	q *= scale;

	v.set((comp_type)q.v.x, (comp_type)q.v.y, (comp_type)q.v.z);
	w = (comp_type)q.w;
}

template<typename T>
void XQuatCompressed<T>::set(const Matrix33<T>& m)
{
	set(Quat<T>(m));
}

template<typename T>
void XQuatCompressed<T>::set(T aW, T x, T y, T z)
{
	set(Quat<T>(aW,x,y,z));
}

// -----------------------------------------------


template<class T>
Quat<T> XQuatCompressed<T>::asQuat() const
{
	Quat<T> q;
	q.set(w,v.x,v.y,v.z);

	q.v /= 32766.5;
	q.w /= 32766.5;

	return q;
}

template<class T>
Matrix33<T> XQuatCompressed<T>::asMatrix33() const
{
	asQuat().toMatrix33();
}

template<typename T>
XQuatCompressed<T>& XQuatCompressed<T>::operator=(const XQuatCompressed<T> &rhs)
{
	v = rhs.v;
	w = rhs.w;
	return *this;
}

template<typename T>
bool XQuatCompressed<T>::operator==(const XQuatCompressed<T> &rhs) const
{
	return v = rhs.v && w == rhs.w;
}

template<typename T>
bool XQuatCompressed<T>::operator!=(const XQuatCompressed<T> &rhs) const
{
	return !(*this == rhs);
}