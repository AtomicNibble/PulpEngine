#pragma once

#ifndef _X_MATH_QUAT_H_
#define _X_MATH_QUAT_H_

#include "XMath.h"
#include "XVector.h"

#include "XMatrix33.h"
#include "XMatrix34.h"
#include "XMatrix44.h"

template<typename T, typename Y>
struct QUATCONV {
	typedef typename T::TYPE F;
	static F	getW(const Y &v) { return static_cast<F>(v.w); }
	static F	getX(const Y &v) { return static_cast<F>(v.x); }
	static F	getY(const Y &v) { return static_cast<F>(v.y); }
	static F	getZ(const Y &v) { return static_cast<F>(v.z); }
};

template<typename T>
class Quat
{
public:
	typedef T	TYPE;
	typedef T	value_type;

	Vec3<T>		v; // axisOfRotation.normalized() * sin( angleOfRotation / 2 )
	T			w; // cos( angleOfRotation / 2 )

	Quat() : v(0, 0, 0), w(1) {} // default constructor is identity quat
	template<typename FromT>
	explicit Quat(const Quat<FromT>& q) : w(static_cast<T>(q.w)), v(q.v) {}

	Quat(T aW, T x, T y, T z) : w(aW), v(x, y, z) {}
	// construct from axis-angle
	Quat(T _w, const Vec3<T> &vec) : w(_w), v(vec) {}
	Quat(const Vec3<T> &axis, T radians) { set(axis, radians); }
	Quat(const Vec3<T> &from, const Vec3<T> &to) { set(from, to); }
	// create from Euler angles in radians expressed in ZYX rotation order
	Quat(T pitch, T yaw, T roll) { set(pitch, yaw, roll); }
	explicit Quat(const Matrix33<T> &m) { set(m); }
	explicit Quat(const Matrix44<T> &m) { set(m); }
	explicit Quat(const Matrix34<T>& m) { set(m); }

	template<typename Y>
	explicit Quat(const Y &v) : 
		w(QUATCONV<Quat<typename Quat::TYPE>, Y>::getW(v)), 
		v(QUATCONV<typename Quat::TYPE, Y>::getX(v), QUATCONV<typename Quat::TYPE, Y>::getY(v), QUATCONV<typename Quat::TYPE, Y>::getZ(v))
	{}

	// get axis-angle representation's axis
	Vec3<T> getAxis() const;
	// get axis-angle representation's angle in radians
	T getAngle() const;
	T getPitch() const;
	T getYaw() const;
	T getRoll() const;
	T dot(const Quat<T> &quat) const;
	T length() const;
	T lengthSquared() const;
	Vec3<T> getEuler(void) const;
	Vec3<T> getEulerDegrees(void) const;

	Quat<T>& normalize();
	Quat<T> normalized() const;
	Quat<T> log() const;
	Quat<T> exp() const;
	Quat<T> inverse() const;
	Quat<T> inverted() const;
	Quat<T>& invert();
	Quat<T> diff(const Quat& oth) const;

	void set(T aW, T x, T y, T z);
	void set(const Vec3<T> &from, const Vec3<T> &to);
	void set(const Vec3<T> &axis, T radians);
	void set(T pitch, T yaw, T roll);
	void getAxisAngle(Vec3<T> *axis, T *radians) const;
	Matrix33<T> toMatrix33() const;
	Matrix44<T> toMatrix44() const;

	X_INLINE operator Matrix44<T>() const { return toMatrix44(); }

	Quat<T> lerp(T t, const Quat<T> &end) const;
	Quat<T> slerpShortestUnenforced(T t, const Quat<T> &end) const;
	Quat<T> slerp(T t, const Quat<T> &end) const;
	Quat<T> squadShortestEnforced(T t, const Quat<T> &qa, const Quat<T> &qb, const Quat<T> &q2) const;
	Quat<T> squad(T t, const Quat<T> &qa, const Quat<T> &qb, const Quat<T> &q2) const;
	Quat<T> spline(T t, const Quat<T> &q1,
		const Quat<T> &q2, const Quat<T> &q3) const;

	void set(const Matrix33<T> &m);
	void set(const Matrix34<T> &m);
	void set(const Matrix44<T> &m);

	// Operators
	Quat<T>& operator=(const Quat<T> &rhs);

	template<typename FromT>
	Quat<T>& operator=(const Quat<FromT> &rhs);
	const Quat<T> operator+(const Quat<T> &rhs) const;

	// post-multiply operator, similar to matrices, but different from Shoemake
	// Concatenates 'rhs' onto 'this'
	const Quat<T> operator*(const Quat<T> &rhs) const;
	const Quat<T> operator*(T rhs) const;

	// transform a vector by the Quat
	const Vec3<T> operator*(const Vec3<T> &vec) const;
	const Quat<T> operator-(const Quat<T> &rhs) const;
	Quat<T>& operator+=(const Quat<T> &rhs);
	Quat<T>& operator-=(const Quat<T>& rhs);
	Quat<T>& operator*=(const Quat<T> &rhs);
	Quat<T>& operator*=(T rhs);

	Quat<T> operator~() const;
	Quat<T> operator-() const;

	bool compare(const Quat<T>& oth, const T elipson) const;

	bool operator==(const Quat<T> &rhs) const;
	bool operator!=(const Quat<T> &rhs) const;

	X_INLINE T& operator[](unsigned int i) { return (&v.x)[i]; }
	X_INLINE const T& operator[](unsigned int i) const { return (&v.x)[i]; }

	static Quat<T> identity()
	{
		return Quat();
	}

private:
	// From advanced Animation and Rendering
	// Techniques by Watt and Watt, Page 366:
	// computing the inner quadrangle 
	// points (qa and qb) to guarantee tangent
	// continuity.
	static Quat<T> splineIntermediate(const Quat<T> &q0, const Quat<T> &q1, const Quat<T> &q2)
	{
		Quat<T> q1inv = q1.inverted();
		Quat<T> c1 = q1inv * q2;
		Quat<T> c2 = q1inv * q0;
		Quat<T> c3 = (c2.log() + c1.log()) * (T)-0.25;
		Quat<T> qa = q1 * c3.exp();
		return qa.normalized();
	}
};


template<typename T>
inline Vec3<T> operator*(const Vec3<T> &vec, const Quat<T> &q)
{
	T vMult = T(2) * (q.v.x * vec.x + q.v.y * vec.y + q.v.z * vec.z);
	T crossMult = T(2) * q.w;
	T pMult = crossMult * q.w - T(1);

	return Vec3<T>(pMult * vec.x + vMult * q.v.x + crossMult * (q.v.y * vec.z - q.v.z * vec.y),
		pMult * vec.y + vMult * q.v.y + crossMult * (q.v.z * vec.x - q.v.x * vec.z),
		pMult * vec.z + vMult * q.v.z + crossMult * (q.v.x * vec.y - q.v.y * vec.x));
}

typedef Quat<float>		Quatf;
typedef Quat<double>	Quatd;

#include "XQuat.inl"

#endif // !_X_MATH_QUAT_H_
