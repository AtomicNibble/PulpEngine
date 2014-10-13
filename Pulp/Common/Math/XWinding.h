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


X_DECLARE_ENUM(PlaneSide)(ON,FRONT,BACK,CROSS);

class XWinding
{
public:
	XWinding(void);
	explicit XWinding(const int n);								// allocate for n points
	explicit XWinding(const Vec3f* verts, const int numVerts);	// winding from points
	explicit XWinding(const Vec3f& normal, const float dist);	// base winding for plane
	explicit XWinding(const Planef& plane);						// base winding for plane
	explicit XWinding(const XWinding& winding);
	~XWinding(void);


	X_INLINE XWinding&		operator=(const XWinding& winding);
	X_INLINE const Vec3f&	operator[](const int idx) const;
	X_INLINE Vec3f&			operator[](const int idx);

	// add a point to the end of the winding point array
	X_INLINE XWinding&		operator+=(const Vec3f& v);
	X_INLINE void			addPoint(const Vec3f& v);

	X_INLINE int getNumPoints(void) const;
	X_INLINE int getAllocatedSize(void) const;

	bool isTiny(void) const;
	bool isHuge(void) const;

	void clear(void);

	// huge winding for plane, the points go counter clockwise when facing the front of the plane
	// makes a winding for that plane
	void baseForPlane(const Vec3f& normal, const float dist);
	void baseForPlane(const Planef& plane);


	float getArea(void) const;
	Vec3f getCenter(void) const;
	float getRadius(const Vec3f& center) const;
	void getPlane(Vec3f& normal, float& dist) const;
	void getPlane(Planef& plane) const;
	void getBoundingBox(AABB& box) const;

	float planeDistance(const Planef& plane) const;
	PlaneSide::Enum	planeSide(const Planef& plane, const float epsilon = EPSILON) const;

	// cuts off the part at the back side of the plane, returns true if some part was at the front
	// if there is nothing at the front the number of points is set to zero
	bool clipInPlace(const Planef& plane, const float epsilon = EPSILON, const bool keepOn = false);

	XWinding* clip(Planef& plane, const float epsilon = EPSILON, const bool keepOn = false);


private:
	bool EnsureAlloced(int n, bool keep = false);
	bool ReAllocate(int n, bool keep = false);


private:
	// should i do fixed of 6?
	// with pointer to data to allow for bigger ones.
	// or just allocate them all from a pool lol.
	Vec3f*		p;
	int			numPoints;
	int			allocedSize;
};


#include "XWinding.inl"

#endif // !X_WINDING_H_