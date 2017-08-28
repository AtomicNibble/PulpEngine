#pragma once

#ifndef _X_MATH_TRANSFORM_H_
#define _X_MATH_TRANSFORM_H_

#include "XMath.h"
#include "XVector.h"
#include "XTransform.h"



template<typename T>
class Transform
{
public:
	Quat<T>		quat;
	Vec3<T>		pos;

	Transform();
	Transform(const Vec3<T>& vec, const Quat<T>& quat);

	template <typename TOth>
	explicit Transform(const Transform<TOth>& qt);
	explicit Transform(const Matrix34<T>& mat);
	explicit Transform(const Matrix44<T>& mat);

	Transform<T>& operator =(const Transform<T>& qt);

	bool operator==(const Transform<T> &rhs) const;
	bool operator!=(const Transform<T> &rhs) const;

	void set(const Vec3<T> &trans, const Quat<T>& qt);
	void set(const Matrix34<T>& mat);
	void set(const Matrix44<T>& mat);


	void setPosition(const Vec3<T>& vec);
	Vec3<T> getPosition(void);
	const Vec3<T>& getPosition(void) const;

	static Transform<T> identity()
	{
		return Transform();
	}
};



typedef Transform<float>	Transformf;
typedef Transform<double>	Transformd;

#include "XTransform.inl"

#endif // !_X_MATH_TRANSFORM_H_
