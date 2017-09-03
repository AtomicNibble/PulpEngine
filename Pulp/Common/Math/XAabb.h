#pragma once

#ifndef _X_MATH_AABB_H_
#define _X_MATH_AABB_H_

//	AABB - Axis Aligned Boinding Box
//

#include "XSphere.h"
#include "XPlane.h"

class OBB;

class AABB
{
public:
	typedef float32_t type;
	typedef float32_t value_type;
	typedef core::StackString256 StrBuf;

	Vec3f min;
	Vec3f max;

	X_INLINE AABB();
	X_INLINE explicit AABB(float radius);
	X_INLINE AABB(const Vec3f& center, float radius);
	X_INLINE AABB(const Vec3f &min, const Vec3f& max);
	X_INLINE AABB(const AABB& oth);
	AABB(const OBB& oth);

	X_INLINE void set(float radius);
	X_INLINE void set(const Vec3f& center, float radius);
	X_INLINE void set(const Vec3f &min, const Vec3f& max);
	X_INLINE void set(const AABB &oth);
	void set(const OBB &oth);

	X_INLINE void  clear(void);
	X_INLINE bool  IsInfinate(void) const; // returns if it's infinate: the value it's made after clear.
	X_INLINE bool  isEmpty(void) const; // retruns if the box contains anyspace 
	X_INLINE Vec3f center(void) const;	// the center point of the box
	X_INLINE Vec3f size(void) const;	// the size of the box.
	X_INLINE Vec3f halfVec(void) const;	// the size of the box / 2
	X_INLINE float radius(void) const;
	X_INLINE float radiusSqr(void) const;
	X_INLINE float volume(void) const;

	// add to the bounding box.
	X_INLINE void add(const Vec3f& v);
	X_INLINE void add(const Vec3f& v, float radius);
	X_INLINE void add(const AABB& bb);


	// shake that booty box
	X_INLINE void move(const Vec3f& v);

	// expands in both directions.
	X_INLINE void expand(const Vec3f& v);

	// clips the current BB with the provided BB
	X_INLINE void clip(const AABB& bb);

	// Check if this bounding box overlap with bounding box of sphere.
	X_INLINE bool containsPoint(const Vec3f& pos) const;
	// check if a BB fits inside this BB.
	X_INLINE bool containsBox(const AABB& b) const;
	// check if a sphere fits inside.
	X_INLINE bool containsSphere(const Sphere& b) const;
	X_INLINE bool containsSphere(const Vec3f &aCenter, float aRadius) const;


	// checks if the BB's intersect.
	X_INLINE bool intersects(const AABB& b) const;

	// Look how god dam sexy my ascii diagram is.
	//
	//			*-----------*
	//		   /|		   /|
	//		  /	|		  /	|
	//		 /	|		 /  |
	//	  z *-----------*	|
	//		|	| y		|	|
	//		|   *-------|---*
	//		|  /		|  /
	//		| /			| /
	//		|/			|/
	//	    *-----------* x
	//					 \
	//					  \<--- distance
	//					   \
	//					    \
	//					     * (x,y,z)
	//
	// returns the distance to a point from the box.
	X_INLINE float distance(const Vec3f& v) const;
	X_INLINE float distanceSqr(const Vec3f& v) const;

	template<typename T>
	X_INLINE PlaneSide::Enum planeSide(const Plane<T>& plane, const float epsilon = 0.1f) const;

	X_INLINE void toPoints(Vec3f points[8]) const;

	X_INLINE const char* toString(StrBuf& str) const;

	static AABB createTransformedAABB(const Quatf& quat, const AABB& aabb);
	static AABB createTransformedAABB(const Matrix33f& m44, const AABB& aabb);
};


#include "XAabb.inl"


#endif // !_X_MATH_AABB_H_
