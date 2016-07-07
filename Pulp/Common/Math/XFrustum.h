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
X_DECLARE_ENUM(CullType)(EXCLUSION, OVERLAP, INCLUSION);
X_DECLARE_ENUM(PlaneVert)(TLEFT,TRIGHT,BLEFT,BRIGHT);

class XFrustum
{
public:
	XFrustum();

	void SetFrustum(uint32_t nWidth, uint32_t nHeight, float32_t FOV, float32_t nearplane,
		float32_t farpane, float32_t fPixelAspectRatio);

	// set some data
	void setPosition(const Vec3f& pos);
	void setAxis(const Matrix33f& mat);
	void setSize(float dNear, float dFar, float dLeft, float dUp);
	void setFov(float fov);

	// get a goat
	const Vec3f& getPosition(void) const;
	const Matrix33f getAxis(void) const;
	const Matrix34f& getMatrix(void) const;
	Vec3f getCenter(void) const;

	bool isValid(void) const;					// returns true if the frustum is valid
//	float getNearDistance(void) const;			// returns distance to near plane
//	float getFarDistance(void) const;			// returns distance to far plane
	float getLeft(void) const;					// returns left vector length
	float getUp(void) const;					// returns up vector length
	float getFov(void) const;
	float getAspectRatio(void) const;

	float32_t getNearPlane(void) const;
	float32_t getFarPlane(void) const;
	Vec3f getEdgeP(void) const;
	Vec3f getEdgeN(void) const;
	Vec3f getEdgeF(void) const;

	X_INLINE void getNearPlaneCoordinates(Vec3f* pTopLeft, Vec3f* pTopRight,
		Vec3f* pBottomLeft, Vec3f* pBottomRight) const;
	X_INLINE void getProPlaneCoordinates(Vec3f* pTopLeft, Vec3f* pTopRight,
		Vec3f* pBottomLeft, Vec3f* pBottomRight) const;
	X_INLINE void getFarPlaneCoordinates(Vec3f * pTopLeft, Vec3f* pTopRight,
		Vec3f* pBottomLeft, Vec3f* pBottomRight) const;


	// fast culling but might not cull everything outside the frustum
	bool cullPoint(const Vec3f& point) const;

	// AABB
	bool cullAABB_Fast(const AABB& box) const;
	bool cullAABB_Exact(const AABB& box) const;
	CullType::Enum cullAABB_FastT(const AABB& box) const;
	CullType::Enum cullAABB_ExactT(const AABB& box) const;

	// OBB
	bool cullOBB_Fast(const OBB& box) const;
	bool cullOBB_Exact(const OBB& box) const;
	CullType::Enum cullOBB_FastT(const OBB& box) const;
	CullType::Enum cullOBB_ExactT(const OBB& box) const;

	// Sphere
	bool cullSphere_Fast(const Sphere& sphere) const;
	bool cullSphere_Exact(const Sphere& sphere) const;
	CullType::Enum cullSphere_FastT(const Sphere& sphere) const;
	CullType::Enum cullSphere_ExactT(const Sphere& sphere) const;

	void GetFrustumVertices(std::array<Vec3f, 8>& verts) const;
	void GetFrustumVertices(std::array<Vec3f, 12>& verts) const;

	X_INLINE float32_t getFov(void) const;
	X_INLINE float32_t getProjectionRatio(void) const;

	X_INLINE void setAngles(const Vec3f& angles);
	X_INLINE Planef getFrustumPlane(FrustumPlane::Enum pl);
	X_INLINE const Planef& getFrustumPlane(FrustumPlane::Enum pl) const;

private:
	CullType::Enum AdditionalCheck(const AABB& aabb) const;
	CullType::Enum AdditionalCheck(const OBB& obb, float32_t scale) const;


private:
	void UpdateFrustum(void);

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