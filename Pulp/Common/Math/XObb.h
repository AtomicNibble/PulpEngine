#pragma once

#ifndef _X_MATH_OBB_H_
#define _X_MATH_OBB_H_

//	OBB - Oriented Boinding Box
//
//
//
//

#include "XMatrix33.h"
#include "XAabb.h"
#include "XQuat.h"

class OBB
{
public:
	typedef float32_t type;
	typedef float32_t value_type;


	X_INLINE OBB() {}
	X_INLINE OBB(Matrix33f m33, const Vec3f& center, const Vec3f& hlv);
	X_INLINE OBB(Matrix33f m33, const AABB& aabb);
	X_INLINE OBB(Quatf quat, const AABB& aabb);

	X_INLINE void set(Matrix33f m33, const Vec3f& center, const Vec3f& hlv);
	X_INLINE void set(Matrix33f m33, const AABB& aabb);
	X_INLINE void set(Quatf quat, const AABB& aabb);


	X_INLINE Vec3f center() const;	// the center point of the box
	X_INLINE Vec3f size() const;	// the size of the box.
	X_INLINE Vec3f halfVec() const;	// 
	X_INLINE const Matrix33f& orientation() const;	// 


private:
	Matrix33f orientation_;
	Vec3f center_;			// center location
	Vec3f halfLVec_;		// half of the box vec.
};

#include "XObb.inl"

#endif // !_X_MATH_OBB_H_