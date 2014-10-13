#include "EngineCommon.h"
#include "XWinding.h"

#include <Util\FloatIEEEUtil.h>

/*
static const int IEEE_FLT_MANTISSA_BITS = 23;
static const int IEEE_FLT_EXPONENT_BITS = 8;
static const int IEEE_FLT_EXPONENT_BIAS = 127;
static const int IEEE_FLT_SIGN_BIT = 31;
*/
#define	EDGE_LENGTH		0.2f

namespace
{
	static const int MAX_WORLD_COORD = (128 * 1024);
	static const int MIN_WORLD_COORD = (-128 * 1024);
	static const int MAX_WORLD_SIZE = (MAX_WORLD_COORD - MIN_WORLD_COORD);


//	const int SMALLEST_NON_DENORMAL = 1 << IEEE_FLT_MANTISSA_BITS;
//	const int NAN_VALUE = 0x7f800000;
//	const float FLT_SMALLEST_NON_DENORMAL = *reinterpret_cast< const float * >(&SMALLEST_NON_DENORMAL);
	
	#define _alloca16( x )				((void *)((((int)_alloca( (x)+15 )) + 15) & ~15))

	X_DISABLE_WARNING(4756)

	float InvSqrt(float x)
	{
		return (x > core::FloatUtil::FLT_SMALLEST_NON_DENORMAL) ? sqrtf(1.0f / x) : INFINITY;
	}

	X_ENABLE_WARNING(4756)

	void NormalVectors(const Vec3f& vec, Vec3f &left, Vec3f &down)
	{
		float d;

		d = vec.x * vec.x + vec.y * vec.y;
		if (!d) {
			left[0] = 1;
			left[1] = 0;
			left[2] = 0;
		}
		else {
			d = InvSqrt(d);
			left[0] = -vec.y * d;
			left[1] = vec.x * d;
			left[2] = 0;
		}
		down = left.cross(vec);
	}


}

XWinding::XWinding(void) : 
	p(nullptr),
	numPoints(0),
	allocedSize(0)
{

}

// allocate for n points
XWinding::XWinding(const int n) :
	p(nullptr),
	numPoints(0),
	allocedSize(0)
{
	EnsureAlloced(n);
}

// winding from points
XWinding::XWinding(const Vec3f* verts, const int numVerts) :
	p(nullptr),
	numPoints(0),
	allocedSize(0)
{
	int i;

	if (!EnsureAlloced(numVerts)) {
		numPoints = 0;
		return;
	}
	for (i = 0; i < numVerts; i++) {
		p[i] = verts[i];
	}
	numPoints = numVerts;
}

// base winding for plane
XWinding::XWinding(const Vec3f& normal, const float dist) :
	p(nullptr),
	numPoints(0),
	allocedSize(0)
{
	baseForPlane(normal, dist);
}

// base winding for plane
XWinding::XWinding(const Planef& plane) :
	p(nullptr),
	numPoints(0),
	allocedSize(0)
{
	baseForPlane(plane);
}


XWinding::XWinding(const XWinding& winding) :
	p(nullptr),
	numPoints(0),
	allocedSize(0)
{
	int i;
	if (!EnsureAlloced(winding.getNumPoints())) {
		numPoints = 0;
		return;
	}
	for (i = 0; i < winding.getNumPoints(); i++) {
		p[i] = winding[i];
	}
	numPoints = winding.getNumPoints();
}


XWinding::~XWinding(void)
{
	clear();
}

// ----------------------------------------------------------------------

bool XWinding::isTiny(void) const
{
	int		i;
	float	len;
	Vec3f	delta;
	int		edges;

	edges = 0;
	for (i = 0; i < numPoints; i++) {
		delta = p[(i + 1) % numPoints] - p[i];
		len = delta.length();
		if (len > EDGE_LENGTH) {
			if (++edges == 3) {
				return false;
			}
		}
	}
	return true;
}

bool XWinding::isHuge(void) const
{
	int i, j;

	for (i = 0; i < numPoints; i++) {
		for (j = 0; j < 3; j++) {
			if (p[i][j] <= MIN_WORLD_COORD || p[i][j] >= MAX_WORLD_COORD) {
				return true;
			}
		}
	}
	return false;
}


void XWinding::clear(void)
{
	numPoints = 0;
	allocedSize = 0;
	delete[] p;
	p = nullptr;
}


// ----------------------------------------------------------------------


void XWinding::baseForPlane(const Vec3f& normal, const float dist)
{
	Vec3f org, vright, vup;

	org = normal * dist;

	NormalVectors(normal, vup, vright);

	vup *= MAX_WORLD_SIZE;
	vright *= MAX_WORLD_SIZE;

	EnsureAlloced(4);
	numPoints = 4;
	p[0] = org - vright + vup;
	p[1] = org + vright + vup;
	p[2] = org + vright - vup;
	p[3] = org - vright - vup;

}

void XWinding::baseForPlane(const Planef& plane)
{
	baseForPlane(plane.getNormal(), plane.getDistance());
}

// ----------------------------------------------------------------------


float XWinding::getArea(void) const
{
	int i;
	float total;
	Vec3f d1, d2, cross;

	total = 0.0f;
	for (i = 2; i < numPoints; i++) 
	{
		d1 = p[i - 1] - p[0];
		d2 = p[i] - p[0];
		cross = d1.cross(d2);
		total += cross.length();
	}
	return total * 0.5f;
}

Vec3f XWinding::getCenter(void) const
{
	int i;
	Vec3f center;

	center = Vec3f::zero();
	for (i = 0; i < numPoints; i++) {
		center += p[i];
	}
	center *= (1.0f / numPoints);
	return center;
}

float XWinding::getRadius(const Vec3f& center) const
{
	int i;
	float radius, r;
	Vec3f dir;

	radius = 0.0f;
	for (i = 0; i < numPoints; i++) {
		dir = p[i] - center;
		r = dir * dir;
		if (r > radius) {
			radius = r;
		}
	}
	return math<float>::sqrt(radius);
}

void XWinding::getPlane(Vec3f& normal, float& dist) const
{
	Vec3f v1, v2, center;

	if (numPoints < 3) {
		normal = Vec3f::zero();
		dist = 0.0f;
		return;
	}

	center = getCenter();
	v1 = p[0] - center;
	v2 = p[1] - center;
	normal = v2.cross(v1);
	normal.normalize();
	dist = p[0] * normal;
}

void XWinding::getPlane(Planef& plane) const
{
	Vec3f v1, v2;
	Vec3f center;

	if (numPoints < 3) {
		//	plane.set(Vec3f::zero(), 0);
		core::zero_object(plane);
		return;
	}

	center = getCenter();
	v1 = p[0] - center;
	v2 = p[1] - center;
	plane.setNormal(v2.cross(v1));
	plane.setDistance(plane.getNormal() * p[0]);
}

void XWinding::getBoundingBox(AABB& box) const
{
	int i;

	if (!numPoints) {
		box.clear();
		return;
	}

	box.min = box.max = p[0];

	for (i = 1; i < numPoints; i++) 
	{
		if (p[i].x < box.min.x) {
			box.min.x = p[i].x;
		}
		else if (p[i].x > box.max.x) {
			box.max.x = p[i].x;
		}

		if (p[i].y < box.min.y) {
			box.min.y = p[i].y;
		}
		else if (p[i].y > box.max.y) {
			box.max.y = p[i].y;
		}

		if (p[i].z < box.min.z) {
			box.min.z = p[i].z;
		}
		else if (p[i].z > box.max.z) {
			box.max.z = p[i].z;
		}
	}
}

float XWinding::planeDistance(const Planef& plane) const
{
	using namespace core::FloatUtil;

	int		i;
	float	d, min, max;

	min = INFINITY;
	max = -min;
	for (i = 0; i < numPoints; i++) 
	{
		d = plane.distance(p[i]);
		if (d < min) {
			min = d;

			if (isSignBitSet(min) && isSignBitNotSet(max)) {
				return 0.0f;
			}
		}
		if (d > max) {
			max = d;

			if (isSignBitSet(min) && isSignBitNotSet(max)) {
				return 0.0f;
			}
		}
	}
	if (isSignBitNotSet(min)) {
		return min;
	}
	if (isSignBitSet(max)) {
		return max;
	}
	return 0.0f;
}

PlaneSide::Enum	XWinding::planeSide(const Planef& plane, const float epsilon) const
{
	bool	front, back;
	int		i;
	float	d;

	front = false;
	back = false;
	for (i = 0; i < numPoints; i++)
	{
		d = plane.distance(p[i]);
		if (d < -epsilon) {
			if (front) {
				return PlaneSide::CROSS;
			}
			back = true;
			continue;
		}
		else if (d > epsilon) {
			if (back) {
				return PlaneSide::CROSS;
			}
			front = true;
			continue;
		}
	}

	if (back) {
		return PlaneSide::BACK;
	}
	if (front) {
		return PlaneSide::FRONT;
	}
	return PlaneSide::ON;
}

// cuts off the part at the back side of the plane, returns true if some part was at the front
// if there is nothing at the front the number of points is set to zero
bool XWinding::clipInPlace(const Planef& plane, const float epsilon, const bool keepOn)
{
	float*		dists;
	uint8_t*	sides;
	Vec3f*		newPoints;
	int			newNumPoints;
	int			counts[3];
	float		dot;
	int			i, j;
	Vec3f*		p1, *p2;
	Vec3f		mid;
	int			maxpts;

//	assert(this);

	dists = (float*)_alloca((numPoints + 4) * sizeof(float));
	sides = (uint8_t*)_alloca((numPoints + 4) * sizeof(uint8_t));

	core::zero_object(counts);

	// determine sides for each point
	for (i = 0; i < numPoints; i++)
	{
		dists[i] = dot = plane.distance(p[i]);
		if (dot > epsilon) {
		//	sides[i] = PlaneSide::FRONT;
			sides[i] = PlaneSide::FRONT;
		}
		else if (dot < -epsilon) {
		//	sides[i] = PlaneSide::BACK;
			sides[i] = PlaneSide::BACK;
		}
		else {
			sides[i] = PlaneSide::ON;
		}

		counts[sides[i]]++;
	}
	sides[i] = sides[0];
	dists[i] = dists[0];

	// if the winding is on the plane and we should keep it
	if (keepOn && !counts[PlaneSide::FRONT] && !counts[PlaneSide::BACK]) {
		return true;
	}
	// if nothing at the front of the clipping plane
	if (!counts[PlaneSide::FRONT]) {
		numPoints = 0;
		return false;
	}
	// if nothing at the back of the clipping plane
	if (!counts[PlaneSide::BACK]) {
		return true;
	}

	maxpts = numPoints + 4;		// cant use counts[0]+2 because of fp grouping errors

	newPoints = (Vec3f*)_alloca16(maxpts * sizeof(Vec3f));
	newNumPoints = 0;

	for (i = 0; i < numPoints; i++) 
	{
		p1 = &p[i];

		if (newNumPoints + 1 > maxpts) {
			return true;		// can't split -- fall back to original
		}

		if (sides[i] == PlaneSide::ON) {
			newPoints[newNumPoints] = *p1;
			newNumPoints++;
			continue;
		}

		if (sides[i] == PlaneSide::FRONT) {
			newPoints[newNumPoints] = *p1;
			newNumPoints++;
		}

		if (sides[i + 1] == PlaneSide::ON || sides[i + 1] == sides[i]) {
			continue;
		}

		if (newNumPoints + 1 > maxpts) {
			return true;		// can't split -- fall back to original
		}

		// generate a split point
		p2 = &p[(i + 1) % numPoints];

		dot = dists[i] / (dists[i] - dists[i + 1]);
		for (j = 0; j < 3; j++) {
			// avoid round off error when possible
			if (plane.getNormal()[j] == 1.0f) {
				mid[j] = plane.getDistance();
			}
			else if (plane.getNormal()[j] == -1.0f) {
				mid[j] = -plane.getDistance();
			}
			else {
				mid[j] = (*p1)[j] + dot * ((*p2)[j] - (*p1)[j]);
			}
		}

		newPoints[newNumPoints] = mid;
		newNumPoints++;
	}

	if (!EnsureAlloced(newNumPoints, false)) {
		return true;
	}

	numPoints = newNumPoints;
	memcpy(p, newPoints, newNumPoints * sizeof(Vec3f));

	return true;
}


XWinding* XWinding::clip(Planef &plane, const float epsilon, const bool keepOn)
{
	Vec3f*		newPoints;
	int			newNumPoints;
	int			counts[3];
	float		dot;
	int			i, j;
	Vec3f*	p1, *p2;
	Vec3f		mid;
	int			maxpts;

	X_ASSERT_NOT_NULL(this);

	static const int MAX_POINTS_ON_WINDING = 32;

	float	dists[MAX_POINTS_ON_WINDING + 4];
	int		sides[MAX_POINTS_ON_WINDING + 4];

	core::zero_object(counts);


	// determine sides for each point
	for (i = 0; i < numPoints; i++) {
		dists[i] = dot = plane.distance(p[i]);
		if (dot > epsilon) {
			sides[i] = PlaneSide::FRONT;
		}
		else if (dot < -epsilon) {
			sides[i] = PlaneSide::BACK;
		}
		else {
			sides[i] = PlaneSide::ON;
		}
		counts[sides[i]]++;
	}
	sides[i] = sides[0];
	dists[i] = dists[0];

	// if the winding is on the plane and we should keep it
	if (keepOn && !counts[PlaneSide::FRONT] && !counts[PlaneSide::BACK]) {
		return this;
	}
	// if nothing at the front of the clipping plane
	if (!counts[PlaneSide::FRONT]) {
		delete this;
		return NULL;
	}
	// if nothing at the back of the clipping plane
	if (!counts[PlaneSide::BACK]) {
		return this;
	}

	maxpts = numPoints + 4;		// cant use counts[0]+2 because of fp grouping errors

	newPoints = (Vec3f*)_alloca16(maxpts * sizeof(Vec3f));
	newNumPoints = 0;

	for (i = 0; i < numPoints; i++) {
		p1 = &p[i];

		if (newNumPoints + 1 > maxpts) {
			return this;		// can't split -- fall back to original
		}

		if (sides[i] == PlaneSide::ON) {
			newPoints[newNumPoints] = *p1;
			newNumPoints++;
			continue;
		}

		if (sides[i] == PlaneSide::FRONT) {
			newPoints[newNumPoints] = *p1;
			newNumPoints++;
		}

		if (sides[i + 1] == PlaneSide::ON || sides[i + 1] == sides[i]) {
			continue;
		}

		if (newNumPoints + 1 > maxpts) {
			return this;		// can't split -- fall back to original
		}

		// generate a split point
		p2 = &p[(i + 1) % numPoints];

		dot = dists[i] / (dists[i] - dists[i + 1]);
		for (j = 0; j < 3; j++) {
			// avoid round off error when possible
			if (plane.getNormal()[j] == 1.0f) {
				mid[j] = plane.getDistance();
			}
			else if (plane.getNormal()[j] == -1.0f) {
				mid[j] = -plane.getDistance();
			}
			else {
				mid[j] = (*p1)[j] + dot * ((*p2)[j] - (*p1)[j]);
			}
		}

		newPoints[newNumPoints] = mid;
		newNumPoints++;
	}

	if (!EnsureAlloced(newNumPoints, false)) {
		return this;
	}

	numPoints = newNumPoints;
	memcpy(p, newPoints, newNumPoints * sizeof(Vec3f));
	return this;
}



// ----------------------------------------------------------------------


bool XWinding::EnsureAlloced(int n, bool keep)
{
	if (n > allocedSize) {
		return ReAllocate(n, keep);
	}
	return true;
}

bool XWinding::ReAllocate(int n, bool keep)
{
	Vec3f* oldP;

	oldP = p;
//	n = (n + 3) & ~3;	// align up to multiple of four

	n = core::bitUtil::RoundUpToMultiple(n, 4);

	p = new Vec3f[n];
	if (oldP) {
		if (keep) {
			memcpy(p, oldP, numPoints * sizeof(p[0]));
		}
		delete[] oldP;
	}
	allocedSize = n;
	return true;
}