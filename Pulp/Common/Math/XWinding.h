#pragma once


#ifndef X_WINDING_H_
#define X_WINDING_H_

//
//	stores a convex pologon
//
//	used for stuff like turing 6 planes into a box.
//
//
//
//
#include "XVector.h"
#include "XPlane.h"
#include "XAabb.h"

#include <ISerialize.h>

// X_DECLARE_ENUM(PlaneSide)(ON,FRONT,BACK,CROSS);

class XWinding : public core::ISerialize
{
	static const int MAX_POINTS_ON_WINDING = 64;
public:
	XWinding(void);
	explicit XWinding(const int n);								// allocate for n points
	explicit XWinding(const Vec3f* verts, const int numVerts);	// winding from points
	explicit XWinding(const Vec5f* verts, const int numVerts);	// winding from points
	explicit XWinding(const Vec3f& normal, const float dist);	// base winding for plane
	explicit XWinding(const Planef& plane);						// base winding for plane
	explicit XWinding(const XWinding& winding);
	~XWinding(void);


	X_INLINE XWinding&		operator=(const XWinding& winding);
	X_INLINE const Vec5f&	operator[](const int idx) const;
	X_INLINE Vec5f&			operator[](const int idx);

	// add a point to the end of the winding point array
	X_INLINE XWinding&		operator+=(const Vec5f& v);
	X_INLINE XWinding&		operator+=(const Vec3f& v);
	X_INLINE void			addPoint(const Vec5f& v);
	X_INLINE void			addPoint(const Vec3f& v);

	X_INLINE int getNumPoints(void) const;
	X_INLINE int getAllocatedSize(void) const;

	bool isTiny(void) const;
	bool isHuge(void) const;
	void clear(void);
	void print(void) const;


	// huge winding for plane, the points go counter clockwise when facing the front of the plane
	// makes a winding for that plane
	void baseForPlane(const Vec3f& normal, const float dist);
	void baseForPlane(const Planef& plane);

	float getArea(void) const;
	Vec3f getCenter(void) const;
	float getRadius(const Vec3f& center) const;
	void getPlane(Vec3f& normal, float& dist) const;
	void getPlane(Planef& plane) const;
	void GetAABB(AABB& box) const;

	float planeDistance(const Planef& plane) const;
	PlaneSide::Enum	planeSide(const Planef& plane, const float epsilon = EPSILON) const;

	// cuts off the part at the back side of the plane, returns true if some part was at the front
	// if there is nothing at the front the number of points is set to zero
	bool clipInPlace(const Planef& plane, const float epsilon = EPSILON, const bool keepOn = false);

	// returns false if invalid.
	bool clip(const Planef& plane, const float epsilon = EPSILON, const bool keepOn = false);
	XWinding* Copy(void) const;
	XWinding* ReverseWinding(void);

	int Split(const Planef &plane, const float epsilon, XWinding **front, XWinding **back) const;


	void AddToConvexHull(const XWinding *winding, const Vec3f& normal, const float epsilon = EPSILON);
	void AddToConvexHull(const Vec3f& point, const Vec3f& normal, const float epsilon = EPSILON);

	// ISerialize
	virtual bool SSave(core::XFile* pFile) const X_FINAL;
	virtual bool SLoad(core::XFile* pFile) X_FINAL;
	// ~ISerialize


private:
	// must be inlined to not fuck up alloca
	X_INLINE bool EnsureAlloced(int32_t num, bool keep = false);
	X_INLINE bool ReAllocate(int32_t num, bool keep = false);

private:
	Vec5f*	pPoints_;
	int32_t	numPoints_;
	int32_t	allocedSize_;
};


#include "XWinding.inl"

#endif // !X_WINDING_H_