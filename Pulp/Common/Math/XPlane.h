#pragma once

#ifndef _X_MATH_PLANE_H_
#define _X_MATH_PLANE_H_

#include "XVector.h"

//
//
//	+
//	|\
//  | \
//  |  \
//  |  /\
//	| /  \
//  |/    \
//  +------+
//
//	A plane 3 points in 3d space.
//  we need 3 points so that the orientation(pitch, yaw, roll)
//  can be determinied.
//
// We store it as a normal and ditance as describied in Methord 3
// http://en.wikipedia.org/wiki/Plane_(geometry)
//
//


X_DECLARE_ENUM(PlaneSide)(FRONT, BACK, ON, CROSS);

struct PlaneType
{
	enum Enum
	{
		X,
		Y,
		Z, 
		NEGX, 
		NEGY,
		NEGZ, 
		ZEROX, 
		ZEROY, 
		ZEROZ, 
		NONAXIAL
	};

	static X_INLINE bool isTrueAxial(Enum type) {
		return type < ZEROX;
	}
};

template<typename T>
class Plane
{
public:
	typedef core::StackString<128,char> Description;

public:
	X_INLINE Plane() : Distance_(static_cast<T>(0)) {}

	X_INLINE Plane(const Vec3<T> &v1, const Vec3<T> &v2, const Vec3<T> &v3) {
		set(v1, v2, v3);
	}
	X_INLINE Plane(const Vec3<T> &point, const Vec3<T> &normal) {
		set(point, normal);
	}
	X_INLINE Plane(const Vec3<T> &normal, const T disatnace) {
		set(normal, disatnace);
	}
	X_INLINE Plane(T a, T b, T c, T d) {
		set(a, b, c, d);
	}

	//! Defines a plane using 3 points. 
	X_INLINE void	set(const Vec3<T> &v1, const Vec3<T> &v2, const Vec3<T> &v3) {
		Normal_ = (v1 - v2).cross(v3 - v2);
		Normal_.normalizeSafe();
		Distance_ = -(Normal_ * v2);
	}
	//! Defines a plane using a normal vector and a point.
	X_INLINE void	set(const Vec3<T> &point, const Vec3<T> &normal) {
		Normal_ = normal.normalized();
		Distance_ = -(Normal_.dot(point));
	}
	//! Defines a plane using a normal and distance
	X_INLINE void	set(const Vec3<T> &normal, const T disatnace) {
		Normal_ = normal.normalized();
		Distance_ = disatnace;
	}
	//! Defines a plane using 4 coefficients.
	X_INLINE void	set(T a, T b, T c, T d) {
		Vec3<T> normal(a, b, c);
		T length = normal.length();

		Normal_ = normal.normalized();
		Distance_ = d / length;
	}

	X_INLINE T					operator[](size_t idx) const;
	X_INLINE T &				operator[](size_t idx);

	X_INLINE Plane				operator-() const {
		return Plane(-Normal_, -Distance_);
	}

	X_INLINE Vec3<T>			getPoint() const { return Normal_ * getDistance(); };
	X_INLINE const Vec3<T>&		getNormal() const { return Normal_; };
	X_INLINE void				setNormal(const Vec3<T>& normal) { Normal_ = normal; }
	X_INLINE T					getDistance() const { return -Distance_; }
	X_INLINE void				setDistance(const float distance) { Distance_ = -distance; }
	X_INLINE T					distance(const Vec3<T> &p) const { return Normal_.dot(p) + Distance_; };

	X_INLINE Vec3<T>			reflectPoint(const Vec3<T> &p) const { return Normal_ * distance(p) * -2 + p; }
	X_INLINE Vec3<T>			reflectVector(const Vec3<T> &v) const { return Normal_ * Normal_.dot(v) * 2 - v; }

	X_INLINE bool				rayIntersection(const Ray& ray, Vec3f& out);

	X_INLINE bool				compare(const Plane& p, const float normalEps, const float distEps) const;

	X_INLINE T					dot(Plane<T>& oth) const {
		return Normal_.dot(oth.Normal_);
	}

	X_INLINE PlaneSide::Enum side(const Vec3<T>& v, const float epsilon) const
	{
		float dist = distance(v);
		if (dist > epsilon) {
			return PlaneSide::FRONT;
		}
		else if (dist < -epsilon) {
			return PlaneSide::BACK;
		}
		
		return PlaneSide::ON;
	}

	PlaneType::Enum getType(void) const
	{
		if (Normal_[0] == 0.0f) 
		{
			if (Normal_[1] == 0.0f) {
				return Normal_[2] > 0.0f ? PlaneType::Z : PlaneType::NEGZ;
			}
			else if (Normal_[2] == 0.0f) {
				return Normal_[1] > 0.0f ? PlaneType::Y : PlaneType::NEGY;
			}
			else {
				return PlaneType::ZEROX;
			}
		}
		else if (Normal_[1] == 0.0f) {
			if (Normal_[2] == 0.0f) {
				return Normal_[0] > 0.0f ? PlaneType::X : PlaneType::NEGX;
			}
			else {
				return PlaneType::ZEROY;
			}
		}
		else if (Normal_[2] == 0.0f) {
			return PlaneType::ZEROZ;
		}
	
		return PlaneType::NONAXIAL;
	}

	const char* toString(Description& desc) const
	{
		desc.setFmt("<%g,%g,%g> - %g", Normal_.x, Normal_.y, Normal_.z, Distance_);
		return desc.c_str();
	}


private:
	Vec3<T>		Normal_;
	T			Distance_;
};

template<typename T>
X_INLINE T Plane<T>::operator[](size_t idx) const {
	return Normal_[idx];
}

template<typename T>
X_INLINE T& Plane<T>::operator[](size_t idx) {
	return Normal_[idx];
}

template<typename T>
X_INLINE bool Plane<T>::rayIntersection(const Ray& ray, Vec3f& out)
{
	float cosine = Normal_.dot(ray.getDirection());

	if (cosine == 0.f) // parallel.
		return false;

	float dis = distance(ray.getOrigin());
	float scale = -(dis / cosine);

	out = ray.getOrigin() + (ray.getDirection() * scale);
	return true;
}

template<typename T>
X_INLINE bool Plane<T>::compare(const Plane& p, const float normalEps, const float distEps) const
{
	if (math<float>::abs(Distance_ - p.Distance_) > distEps) {
		return false;
	}
	if (!Normal_.compare(p.getNormal(), normalEps)) {
		return false;
	}
	return true;
}


typedef Plane<float>	Planef;
typedef Plane<double>	Planed;

#endif // !_X_MATH_PLANE_H_
