#pragma once

#ifndef _X_MATH_TRANSFORM_H_
#define _X_MATH_TRANSFORM_H_

#include "XMath.h"
#include "XVector.h"
#include "XTransform.h"



template<typename T>
class QuatTrans
{
public:
	Quat<T>		quat;
	Vec3<T>		trans;

	QuatTrans();
	QuatTrans(const Vec3<T>& vec, const Quat<T>& quat);

	template <typename TOth>
	explicit QuatTrans(const QuatTrans<TOth>& qt);

	explicit QuatTrans(const Matrix34<T>& mat);

	QuatTrans<T>& operator =(const QuatTrans<T>& qt);

	bool operator==(const QuatTrans<T> &rhs) const;
	bool operator!=(const QuatTrans<T> &rhs) const;

	void set(const Vec3<T> &trans, const Quat<T>& qt);
	void set(const Matrix34<T>& mat);


	void setTranslation(const Vec3<T>& vec);
	Vec3<T> getTranslation(void) const;

	static QuatTrans<T> identity()
	{
		return QuatTrans();
	}
};



typedef QuatTrans<float>	QuatTransf;
typedef QuatTrans<double>	QuatTransd;

#include "XTransform.inl"

#endif // !_X_MATH_TRANSFORM_H_
