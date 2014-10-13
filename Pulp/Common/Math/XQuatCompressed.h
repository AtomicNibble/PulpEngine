#pragma once


#ifndef _X_MATH_QUAT_COMPRESSED_H_
#define _X_MATH_QUAT_COMPRESSED_H_

#include "XQuat.h"


template<typename T>
class XQuatCompressed
{
public:
	typedef T	TYPE;
	typedef T	value_type;
	typedef int16_t comp_type;

	Vec3<comp_type>		v;
	int16_t				w;

	XQuatCompressed() : w(0) {};
	XQuatCompressed(const Quat<T>& q);
	XQuatCompressed(const Matrix33<T>& m);
	XQuatCompressed(T aW, T x, T y, T z);

	void set(const Quat<T>& q);
	void set(const Matrix33<T>& m);
	void set(T aW, T x, T y, T z);


	Quat<T>	asQuat() const;
	Matrix33<T> asMatrix33() const;

	// Operators
	XQuatCompressed<T>& operator=(const XQuatCompressed<T> &rhs);

	bool operator==(const XQuatCompressed<T> &rhs) const;
	bool operator!=(const XQuatCompressed<T> &rhs) const;

	X_INLINE T& operator[](unsigned int i) { return (&v.x)[i]; }
	X_INLINE const T& operator[](unsigned int i) const { return (&v.x)[i]; }

	static XQuatCompressed<T> identity()
	{
		return XQuatCompressed();
	}

private:

};

#include "XQuatCompressed.inl"

typedef XQuatCompressed<float32_t> XQuatCompressedf;
typedef XQuatCompressed<float64_t> XQuatCompressedd;

#endif // !_X_MATH_QUAT_COMPRESSED_H_
