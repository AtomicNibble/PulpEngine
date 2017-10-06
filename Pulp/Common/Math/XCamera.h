#pragma once

#ifndef _X_MATH_CAMERA_H_
#define _X_MATH_CAMERA_H_

// i'm gonna do a Z-axis up system since it's what I'm use to.
// Maya / cod etc..
// Diagram:
//
//	 Z-axis
//	 ^
//	 |
//	 |   Y-axis
//	 |   /
//	 |  /
//	 | /
//	 |/
//	 *-------------> X-axis
//	 ^ sexy
//
//  We only really need a 34 matrix.
//  3 rows for the Up, Look, Right vectors
//  and 1 for the position.
//
//	3x4 Matrix:
//	+------------ + ------------ + ------------ + ------------ +
//	| Right		  | Up			 | Look			| Position	   |
//	+------------ + ------------ + ------------ + ------------ +
//	| Right.x	  | Up.x		 | Look.x		| position.x   |
//	| Right.y	  | Up.y		 | Look.y		| position.y   |
//	| Right.z	  | Up.z		 | Look.z		| position.z   |
//	+------------ + ------------ + ------------ + ------------ +
//
// Directx: http://msdn.microsoft.com/en-us/library/windows/desktop/bb204853(v=vs.85).aspx
//
//
//	We are going to store the Frustum planes, Near & Far
//  we also store the R,L,T,B
//
//  And we have 3 planes Near, Projection, Far
//  meaning the near plane can be behind the projection
//  so that objects won't be clipped when hitting projection.
//
// 
//
//

#include "XMath.h"

static const float  DEFAULT_NEAR	= 0.25f;
static const float  DEFAULT_FAR		= 1024.0f;
static const float  DEFAULT_FOV		= toRadians(75.0f);

//////////////////////////////////////////////////////////////////////

#ifdef NEAR
#undef NEAR
#endif

#ifdef FAR
#undef FAR
#endif

// X_DECLARE_ENUM(FrustumPlanes)(NEAR,FAR,RIGHT,LEFT,TOP,BOTTOM);

#include "XPlane.h"
#include "XMatrix.h"
#include "XFrustum.h"

class XCamera : public XFrustum
{
public:
	XCamera() = default;
	~XCamera() = default;

	X_INLINE void setFrustum(uint32_t width, uint32_t height,
		float32_t fov = DEFAULT_FOV, float32_t nearplane = DEFAULT_NEAR, 
		float32_t farpane = DEFAULT_FAR, float32_t pixelAspectRatio = 1.0f);

	X_INLINE const Matrix44f& getProjectionMatrix(void) const;
	X_INLINE const Matrix44f& getViewMatrix(void) const;

private:
	void UpdateFrustum(void) X_OVERRIDE;

private:
	Matrix44f projectionMatrix_;
	Matrix44f viewMatrix_;
};


X_INLINE void XCamera::setFrustum(uint32_t nWidth, uint32_t nHeight, float32_t FOV,
	float32_t nearplane, float32_t farpane, float32_t fPixelAspectRatio)
{
	XFrustum::setFrustum(nWidth, nHeight, FOV,
		nearplane, farpane, fPixelAspectRatio);
}


X_INLINE const Matrix44f& XCamera::getProjectionMatrix(void) const
{
	return projectionMatrix_;
}

X_INLINE const Matrix44f& XCamera::getViewMatrix(void) const
{
	return viewMatrix_;
}



#endif // !_X_MATH_CAMERA_H_
