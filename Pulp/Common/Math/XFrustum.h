#pragma once


#ifndef X_MATH_FRUSTUM_H_
#define X_MATH_FRUSTUM_H_


#include "XMatrix33.h"
#include "XMatrix34.h"
#include "XPlane.h"

#include "XAabb.h"
#include "XObb.h"
#include "XSphere.h"

#include <array>

// class AABB;
// class Sphere;
// class OBB;
class Ray;

// GO AWAY!
#ifdef NEAR
#undef NEAR
#endif
#ifdef FAR
#undef FAR
#endif


X_DECLARE_ENUM(FrustumPlane)(NEAR,FAR,RIGHT,LEFT,TOP,BOTTOM);
X_DECLARE_ENUM(CullResult)(EXCLUSION, OVERLAP, INCLUSION);
X_DECLARE_ENUM(PlaneVert)(TLEFT,TRIGHT,BLEFT,BRIGHT);

class XFrustum
{
public:
	typedef std::array<Vec3f, 8> FarNearVertsArr;
	typedef std::array<Vec3f, 12> FarProNearVertsArr;

public:
	XFrustum();

	void setFrustum(uint32_t nWidth, uint32_t nHeight, float32_t FOV, float32_t nearplane,
		float32_t farpane, float32_t fPixelAspectRatio);

	// set some data
	X_INLINE void set(const Matrix33f& axis, const Vec3f& pos);
	X_INLINE void setSize(float dNear, float dFar, float dLeft, float dUp);
	void setFov(float fov);

	// get a goat
	X_INLINE Vec3f getPosition(void) const;
	X_INLINE Matrix33f getAxis(void) const;
	X_INLINE const Matrix34f& getMatrix(void) const;
	X_INLINE Vec3f getCenter(void) const;

	X_INLINE bool isValid(void) const;					// returns true if the frustum is valid
	X_INLINE float32_t getLeft(void) const;					// returns left vector length
	X_INLINE float32_t getUp(void) const;					// returns up vector length
	X_INLINE float32_t getFov(void) const;
	X_INLINE float32_t getAspectRatio(void) const;
	X_INLINE float32_t getProjectionRatio(void) const;

	X_INLINE float32_t getNearPlane(void) const;
	X_INLINE float32_t getFarPlane(void) const;
	X_INLINE const Vec3f& getEdgeP(void) const;
	X_INLINE const Vec3f& getEdgeN(void) const;
	X_INLINE const Vec3f& getEdgeF(void) const;

	X_INLINE const Planef& getFrustumPlane(FrustumPlane::Enum pl) const;

	// fast culling but might not cull everything outside the frustum
	bool cullPoint(const Vec3f& point) const;

	// AABB
	bool cullAABB_Fast(const AABB& box) const;
	bool cullAABB_Exact(const AABB& box) const;
	CullResult::Enum cullAABB_FastT(const AABB& box) const;
	CullResult::Enum cullAABB_ExactT(const AABB& box) const;

	// OBB
	bool cullOBB_Fast(const OBB& box) const;
	bool cullOBB_Exact(const OBB& box) const;
	CullResult::Enum cullOBB_FastT(const OBB& box) const;
	CullResult::Enum cullOBB_ExactT(const OBB& box) const;

	// Sphere
	bool cullSphere_Fast(const Sphere& sphere) const;
	bool cullSphere_Exact(const Sphere& sphere) const;
	CullResult::Enum cullSphere_FastT(const Sphere& sphere) const;
	CullResult::Enum cullSphere_ExactT(const Sphere& sphere) const;

	void GetFrustumVertices(FarNearVertsArr& verts) const;
	void GetFrustumVertices(FarProNearVertsArr& verts) const;

	void getNearPlaneCoordinates(Vec3f* pTopLeft, Vec3f* pTopRight,
		Vec3f* pBottomLeft, Vec3f* pBottomRight) const;
	void getProPlaneCoordinates(Vec3f* pTopLeft, Vec3f* pTopRight,
		Vec3f* pBottomLeft, Vec3f* pBottomRight) const;
	void getFarPlaneCoordinates(Vec3f * pTopLeft, Vec3f* pTopRight,
		Vec3f* pBottomLeft, Vec3f* pBottomRight) const;

private:
	CullResult::Enum additionalCheck(const AABB& aabb) const;
	CullResult::Enum additionalCheck(const OBB& obb, float32_t scale) const;


protected:
	virtual void UpdateFrustum(void);

protected:
	// pos + ang
	Matrix34f mat_;
	float near_;
	float far_;
	float left_;
	float up_;
	float invFar_;		// 1.0f / dFar
	
	uint32_t width_;
	uint32_t height_;
	float fov_;
	float projectionRatio_;
	float pixelAspectRatio_;


	// this is the left/upper vertex of the near-plane (local-space)
	Vec3f	edge_nlt_;					
	// this is the left/upper vertex of the projection-plane (local-space)	
	Vec3f	edge_plt_;					
	// this is the left/upper vertex of the far-clip-plane (local-space)
	Vec3f	edge_flt_;					


	Planef	planes_[FrustumPlane::ENUM_COUNT]; //

	uint32_t idx_[FrustumPlane::ENUM_COUNT];
	uint32_t idy_[FrustumPlane::ENUM_COUNT];
	uint32_t idz_[FrustumPlane::ENUM_COUNT];

	//this are the 4 vertices of the projection-plane in cam-space
	Vec3f proVerts[PlaneVert::ENUM_COUNT];
	//this are the 4 vertices of the near-plane in cam-space
	Vec3f npVerts[PlaneVert::ENUM_COUNT];
	//this are the 4 vertices of the farclip-plane in cam-space
	Vec3f fpVerts[PlaneVert::ENUM_COUNT];
	

private:

};


#include "XFrustum.inl"

#endif // X_MATH_FRUSTUM_H_