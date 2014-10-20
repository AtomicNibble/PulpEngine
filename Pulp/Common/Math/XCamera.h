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

static const float  DEFAULT_NEAR	= 0.25f;
static const float  DEFAULT_FAR		= 1024.0f;
static const float  DEFAULT_FOV		= (75.0f*((float32_t)PI) / 180.0f);

//////////////////////////////////////////////////////////////////////

#ifdef NEAR
#undef NEAR
#endif

#ifdef FAR
#undef FAR
#endif

X_DECLARE_ENUM(FrustumPlanes)(NEAR,FAR,RIGHT,LEFT,TOP,BOTTOM);

#include "XPlane.h"
#include "XMatrix.h"

class XCamera
{
public:
	XCamera()	{ 
		matrix_.setToIdentity();
		SetFrustum(640, 480); 
	}
	~XCamera() {}

	X_INLINE void SetFrustum(int nWidth, int nHeight, float32_t FOV = DEFAULT_FOV,
		float32_t nearplane = DEFAULT_NEAR, float32_t farpane = DEFAULT_FAR, float32_t fPixelAspectRatio = 1.0f);

	X_INLINE void UpdateFrustum();

	X_INLINE int GetViewSurfaceX() const { return width_; }
	X_INLINE int GetViewSurfaceZ() const { return height_; }

	X_INLINE float32_t GetFov() const { return fov_; }

	X_INLINE Matrix34f GetMatrix() const { return matrix_; }
	X_INLINE Matrix34f GetViewMatrix() const { return matrix_.inverted(); }
	X_INLINE Vec3f GetPosition() const { return matrix_.getTranslate(); }
	X_INLINE void SetPosition(const Vec3f& vec) { matrix_.setTranslate(vec); UpdateFrustum(); }

	X_INLINE void SetAngles(const Vec3f& angles)	{
		matrix_ = Matrix34f::createRotation(angles);
		UpdateFrustum();
	}


	// planes
	X_INLINE float32_t GetNearPlane() const { return edge_nlt_.y; }
	X_INLINE float32_t GetFarPlane() const { return edge_flt_.y; }
	X_INLINE float32_t GetProjectionRatio() const { return projectionRatio_; }

	X_INLINE Planef GetFrustumPlane(FrustumPlanes::Enum pl) {
		return fp_[pl];
	}

	X_INLINE const Planef& GetFrustumPlane(FrustumPlanes::Enum pl) const {
		return fp_[pl];
	}

protected:

	Matrix34f matrix_;

	int width_;
	int height_;

	float32_t fov_;

	float32_t projectionRatio_;	// ratio between width and height of view-surface
	float32_t pixelAspectRatio_;	// accounts for aspect ratio and non-square pixels

	Vec3f	edge_nlt_;			// this is the left/upper vertex of the near-plane
	Vec3f	edge_plt_;			// this is the left/upper vertex of the projection-plane 
	Vec3f	edge_flt_;			// this is the left/upper vertex of the far-clip-plane


	Planef		fp_[FrustumPlanes::ENUM_COUNT];

	Vec3f	cltp_, crtp_, clbp_, crbp_;		//this are the 4 vertices of the projection-plane in cam-space
	Vec3f	cltn_, crtn_, clbn_, crbn_;		//this are the 4 vertices of the near-plane in cam-space
	Vec3f	cltf_, crtf_, clbf_, crbf_;		//this are the 4 vertices of the farclip-plane in cam-space

};


// Frustum diagram: http://winpic.co/152d7037
// goat: http://winpic.co/153f6eb6
// --------------------------------------------
X_INLINE void XCamera::SetFrustum(int nWidth, int nHeight, float32_t FOV,
		float32_t nearplane, float32_t farpane, float32_t fPixelAspectRatio)
{
	X_ASSERT(nearplane > 0.001f, "near plane not valid")(nearplane);
	X_ASSERT(farpane > 0.1f, "far plane not valid")(farpane);
	X_ASSERT(farpane > nearplane, "near plane is less than far plane")(farpane, nearplane);

	width_ = nWidth;
	height_ = nHeight; 

	fov_ = FOV;

	float32_t fWidth = (((float32_t)nWidth) / fPixelAspectRatio);
	float32_t fHeight = (float32_t)nHeight;

	projectionRatio_ = fWidth / fHeight;    // projection ratio (1.0 for square pixels)
	pixelAspectRatio_ = fPixelAspectRatio;

	float32_t projLeftTopX = -fWidth*0.5f;
	float32_t projLeftTopY = (float32_t)((1.0f / math<float32_t>::tan(fov_ * 0.5f)) * (fHeight * 0.5f));
	float32_t projLeftTopZ = fHeight * 0.5f;


	edge_plt_.x = projLeftTopX;
	edge_plt_.y = projLeftTopY;
	edge_plt_.z = projLeftTopZ;
//	assert(fabs(acos_tpl(Vec3r(0, m_edge_plt.y, m_edge_plt.z).GetNormalized().y) * 2 - m_fov)<0.001);

	float invProjLeftTopY = 1.0f / projLeftTopY;
	edge_nlt_.x = nearplane * projLeftTopX * invProjLeftTopY;
	edge_nlt_.y = nearplane;
	edge_nlt_.z = nearplane * projLeftTopZ * invProjLeftTopY;

	//calculate the left/upper edge of the far-plane (=not rotated) 
	edge_flt_.x = projLeftTopX  * (farpane * invProjLeftTopY);
	edge_flt_.y = farpane;
	edge_flt_.z = projLeftTopZ  * (farpane * invProjLeftTopY);

	UpdateFrustum();
}

X_INLINE void XCamera::UpdateFrustum()
{
	//-------------------------------------------------------------------
	//--- calculate frustum-edges of projection-plane in CAMERA-SPACE ---
	//-------------------------------------------------------------------
	Matrix33f m33 = matrix_.subMatrix33(0,0);

	cltp_ = m33*Vec3f(+edge_plt_.x, +edge_plt_.y, +edge_plt_.z);
	crtp_ = m33*Vec3f(-edge_plt_.x, +edge_plt_.y, +edge_plt_.z);
	clbp_ = m33*Vec3f(+edge_plt_.x, +edge_plt_.y, -edge_plt_.z);
	crbp_ = m33*Vec3f(-edge_plt_.x, +edge_plt_.y, -edge_plt_.z);

	cltn_ = m33*Vec3f(+edge_nlt_.x, +edge_nlt_.y, +edge_nlt_.z);
	crtn_ = m33*Vec3f(-edge_nlt_.x, +edge_nlt_.y, +edge_nlt_.z);
	clbn_ = m33*Vec3f(+edge_nlt_.x, +edge_nlt_.y, -edge_nlt_.z);
	crbn_ = m33*Vec3f(-edge_nlt_.x, +edge_nlt_.y, -edge_nlt_.z);

	cltf_ = m33*Vec3f(+edge_flt_.x, +edge_flt_.y, +edge_flt_.z);
	crtf_ = m33*Vec3f(-edge_flt_.x, +edge_flt_.y, +edge_flt_.z);
	clbf_ = m33*Vec3f(+edge_flt_.x, +edge_flt_.y, -edge_flt_.z);
	crbf_ = m33*Vec3f(-edge_flt_.x, +edge_flt_.y, -edge_flt_.z);

	const Vec3f& position = GetPosition();

	// set the 3 points for each plane.
	fp_[FrustumPlanes::NEAR].set(crtn_ + position, cltn_ + position, crbn_ + position);
	fp_[FrustumPlanes::RIGHT].set(crbf_ + position, crtf_ + position, position);
	fp_[FrustumPlanes::LEFT].set(cltf_ + position, clbf_ + position, position);
	fp_[FrustumPlanes::TOP].set(crtf_ + position, cltf_ + position, position);
	fp_[FrustumPlanes::BOTTOM].set(clbf_ + position, crbf_ + position, position);
	fp_[FrustumPlanes::FAR].set(crtf_ + position, crbf_ + position, cltf_ + position);  //clip-plane


}

#endif // !_X_MATH_CAMERA_H_
