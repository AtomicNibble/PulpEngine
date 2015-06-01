#pragma once

#ifndef X_WINDING_H_
#define X_WINDING_H_


/*
#define	SIDE_FRONT					0
#define	SIDE_BACK					1
#define	SIDE_ON						2
#define	SIDE_CROSS					3
*/

// make a type safe tmepalted version?
template<typename T>
T* Alloca16(size_t num) 
{
	void* pData = _alloca(num + 15);
	// align.
	pData = core::pointerUtil::AlignTop(pData,16);
	return reinterpret_cast<T*>(pData);
}


#define	MAX_POINTS_ON_WINDING	64
#define	EDGE_LENGTH		0.2f

class XWinding
{
public:
	XWinding(void);
	explicit XWinding(const int n);								// allocate for n points
	explicit XWinding(const Vec3f *verts, const int n);			// winding from points
	explicit XWinding(const Vec3f &normal, const float dist);	// base winding for plane
	explicit XWinding(const Planef &plane);						// base winding for plane
	explicit XWinding(const XWinding &winding);
	~XWinding(void);

	XWinding &		operator=(const XWinding &winding);
	const Vec5f &	operator[](const int index) const;
	Vec5f &			operator[](const int index);


	// number of points on winding
	int				GetNumPoints(void) const;
	void			SetNumPoints(int n);
	virtual void	Clear(void);
	void			Print(void) const;

	// huge winding for plane, the points go counter clockwise when facing the front of the plane
	void			BaseForPlane(const Vec3f &normal, const float dist);
	void			BaseForPlane(const Planef &plane);

	float			GetArea(void) const;
	Vec3f			GetCenter(void) const;
	float			GetRadius(const Vec3f &center) const;
	void			GetPlane(Vec3f &normal, float &dist) const;
	void			GetPlane(Planef &plane) const;
	void			GetAABB(AABB& bounds) const;

	float			PlaneDistance(const Planef &plane) const;
	Planeside::Enum PlaneSide(const Planef &plane, const float epsilon = ON_EPSILON) const;

	int				Split(const Planef &plane, const float epsilon, XWinding **front, XWinding **back) const;
	// returns the winding fragment at the front of the clipping plane,
	// if there is nothing at the front the winding itself is destroyed and NULL is returned
	XWinding *		Clip(const Planef& plane, const float epsilon = ON_EPSILON, const bool keepOn = false);
	XWinding *		Copy(void) const;



	void			AddToConvexHull(const XWinding *winding, const Vec3f& normal, const float epsilon = ON_EPSILON);
	// add a point to the convex hull
	void			AddToConvexHull(const Vec3f& point, const Vec3f& normal, const float epsilon = ON_EPSILON);

	bool IsTiny(void) const;
	bool IsHuge(void) const;


	static float	TriangleArea(const Vec3f &a, const Vec3f &b, const Vec3f &c) {
		Vec3f	v1, v2;
		Vec3f	cross;

		v1 = b - a;
		v2 = c - a;
		cross = v1.cross(v2);
		return 0.5f * cross.length();
	}

	XWinding* ReverseWinding(void);

protected:
	int				numPoints;				// number of points
	Vec5f *			p;						// pointer to point data
	int				allocedSize;

	bool			EnsureAlloced(int n, bool keep = false);
	bool			ReAllocate(int n, bool keep = false);
};


X_INLINE XWinding::XWinding(void)
{
	numPoints = allocedSize = 0;
	p = nullptr;
}

X_INLINE XWinding::XWinding(const int n)
{
	numPoints = allocedSize = 0;
	p = nullptr;
	EnsureAlloced(n);
}

X_INLINE XWinding::XWinding(const Vec3f *verts, const int n)
{
	int i;

	numPoints = allocedSize = 0;
	p = nullptr;
	if (!EnsureAlloced(n)) {
		numPoints = 0;
		return;
	}
	for (i = 0; i < n; i++)
	{
		p[i].asVec3() = verts[i];
		p[i].s = p[i].t = 0.0f;
	}
	numPoints = n;
}

X_INLINE XWinding::XWinding(const Vec3f &normal, const float dist)
{
	numPoints = allocedSize = 0;
	p = nullptr;
	BaseForPlane(normal, dist);
}

X_INLINE XWinding::XWinding(const Planef &plane)
{
	numPoints = allocedSize = 0;
	p = nullptr;
	BaseForPlane(plane);
}

X_INLINE XWinding::XWinding(const XWinding &winding)
{
	int i;
	if (!EnsureAlloced(winding.GetNumPoints())) {
		numPoints = 0;
		return;
	}
	for (i = 0; i < winding.GetNumPoints(); i++) {
		p[i] = winding[i];
	}
	numPoints = winding.GetNumPoints();
}

X_INLINE XWinding::~XWinding(void)
{
	Clear();
}

// -------------------------------------

X_INLINE XWinding& XWinding::operator=(const XWinding &winding)
{
	int i;

	if (!EnsureAlloced(winding.numPoints)) {
		numPoints = 0;
		return *this;
	}
	for (i = 0; i < winding.numPoints; i++) {
		p[i] = winding.p[i];
	}
	numPoints = winding.numPoints;
	return *this;
}

X_INLINE const Vec5f& XWinding::operator[](const int index) const
{
	return p[index];
}

X_INLINE Vec5f& XWinding::operator[](const int index)
{
	return p[index];
}

// number of points on winding
X_INLINE int XWinding::GetNumPoints(void) const
{
	return numPoints;
}

X_INLINE void XWinding::SetNumPoints(int n)
{
	if (!EnsureAlloced(n, true)) {
		return;
	}
	numPoints = n;
}

X_INLINE void XWinding::Clear(void)
{
	numPoints = 0;
	X_DELETE_ARRAY(p);
	p = nullptr;
}


X_INLINE bool XWinding::IsTiny(void) const
{
	int		i;
	float	len;
	Vec3f	delta;
	int		edges;

	edges = 0;
	for (i = 0; i < numPoints; i++) {
		delta = p[(i + 1) % numPoints].xyz() - p[i].xyz();
		len = delta.length();
		if (len > EDGE_LENGTH) {
			if (++edges == 3) {
				return false;
			}
		}
	}
	return true;
}

/*
=============
idWinding::IsHuge
=============
*/
X_INLINE bool XWinding::IsHuge(void) const
{
	int i, j;

	for (i = 0; i < numPoints; i++) {
		for (j = 0; j < 3; j++) {
			if (p[i][j] <= level::MIN_WORLD_COORD || p[i][j] >= level::MAX_WORLD_COORD) {
				return true;
			}
		}
	}
	return false;
}



#endif // X_WINDING_H_