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

#include <IFileSys.h>
#include <Util\FloatIEEEUtil.h>


class WindingGlobalAlloc
{
public:
	X_INLINE Vec5f* alloc(size_t num)
	{
		return X_NEW_ARRAY(Vec5f, num, gEnv->pArena, "WindingRealoc");;
	}

	X_INLINE void free(Vec5f* pPoints)
	{
		X_DELETE_ARRAY(pPoints, gEnv->pArena);
	}
};


template<class Allocator>
class XWindingT 
{
	static const int MAX_POINTS_ON_WINDING = 64;
	const float EDGE_LENGTH = 0.2f;
	const int MAX_WORLD_COORD = (128 * 1024);
	const int MIN_WORLD_COORD = (-128 * 1024);
	const int MAX_WORLD_SIZE = (MAX_WORLD_COORD - MIN_WORLD_COORD);


	typedef Vec5f Type;
	typedef Type value_type;
	typedef Type* TypePtr;
	typedef const Type* ConstTypePtr;
	typedef Type* Iterator;
	typedef const Type* ConstIterator;
	typedef size_t size_type;
	typedef Type& Reference;
	typedef Type& reference;
	typedef const Type& ConstReference;
	typedef const Type& const_reference;
	typedef XWindingT<Allocator> MyType;

public:
	XWindingT(void);
	explicit XWindingT(const size_t n);								// allocate for n points
	explicit XWindingT(const Vec3f* verts, const size_t numVerts);	// winding from points
	explicit XWindingT(const Vec5f* verts, const size_t numVerts);	// winding from points
	explicit XWindingT(const Vec3f& normal, const float dist);	// base winding for plane
	explicit XWindingT(const Planef& plane);						// base winding for plane
	explicit XWindingT(const MyType& winding);
	explicit XWindingT(MyType&& winding);
	~XWindingT(void);


	X_INLINE MyType&		operator=(const MyType& winding);
	X_INLINE MyType&		operator=(MyType&& winding);
	X_INLINE const Vec5f&	operator[](const size_t idx) const;
	X_INLINE Vec5f&			operator[](const size_t idx);

	X_INLINE const Vec5f&	at(size_t idx) const;
	X_INLINE Vec5f&			at(size_t idx);

	// add a point to the end of the winding point array
	X_INLINE MyType&		operator+=(const Vec5f& v);
	X_INLINE MyType&		operator+=(const Vec3f& v);
	X_INLINE void			addPoint(const Vec5f& v);
	X_INLINE void			addPoint(const Vec3f& v);

	X_INLINE size_t getNumPoints(void) const;
	X_INLINE size_t getAllocatedSize(void) const;

	bool isTiny(void) const;
	bool isHuge(void) const;
	void clear(void);
	void free(void);
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
	MyType* Copy(core::MemoryArenaBase* arena) const;
	MyType* Move(core::MemoryArenaBase* arena);
	MyType* ReverseWinding(core::MemoryArenaBase* arena) const;

	PlaneSide::Enum Split(const Planef& plane, const float epsilon,
		MyType** pFront, MyType** pBack, core::MemoryArenaBase* arena) const;
	PlaneSide::Enum SplitMove(const Planef& plane, const float epsilon,
		MyType** pFront, MyType** pBack, core::MemoryArenaBase* arena);


	void AddToConvexHull(const MyType* pWinding, const Vec3f& normal, const float epsilon = EPSILON);
	void AddToConvexHull(const Vec3f& point, const Vec3f& normal, const float epsilon = EPSILON);

	bool SSave(core::XFile* pFile) const;
	bool SLoad(core::XFile* pFile);

	inline TypePtr ptr(void);
	inline ConstTypePtr ptr(void) const;
	inline TypePtr data(void);
	inline ConstTypePtr data(void) const;

	inline Iterator begin(void);
	inline ConstIterator begin(void) const;
	inline Iterator end(void);
	inline ConstIterator end(void) const;

private:
	// must be inlined to not fuck up alloca
	X_INLINE void EnsureAlloced(size_t num, bool keep = false);
	X_INLINE void ReAllocate(int32_t num, bool keep = false);

public:
	static float TriangleArea(const Vec3f& a, const Vec3f& b, const Vec3f& c);
	static void NormalVectors(const Vec3f& vec, Vec3f &left, Vec3f &down);


private:
	Allocator allocator_;
	Vec5f*	pPoints_;
	int32_t	numPoints_;
	int32_t	allocedSize_;
};

typedef XWindingT<WindingGlobalAlloc> XWinding;
#define alloca16(numBytes) ((void *)((((uintptr_t)_alloca( (numBytes)+15 )) + 15) & ~15))

#include "XWinding.inl"

#undef alloca16


#endif // !X_WINDING_H_