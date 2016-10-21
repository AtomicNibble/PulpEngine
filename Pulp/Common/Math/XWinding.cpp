#include "EngineCommon.h"
#include "XWinding.h"

#include <IFileSys.h>
#include <Util\FloatIEEEUtil.h>

#define	EDGE_LENGTH		0.2f

namespace
{
	static const int MAX_WORLD_COORD = (128 * 1024);
	static const int MIN_WORLD_COORD = (-128 * 1024);
	static const int MAX_WORLD_SIZE = (MAX_WORLD_COORD - MIN_WORLD_COORD);

#if 1
	#define alloca16(numBytes) ((void *)((((uintptr_t)_alloca( (numBytes)+15 )) + 15) & ~15))
#else
	template<typename T>
	X_INLINE T* Alloca16(size_t num)
	{
		void* pData = _alloca(num + 15);
		// align.
		pData = core::pointerUtil::AlignTop(pData, 16);
		return reinterpret_cast<T*>(pData);
	}
#endif

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
	pPoints_(nullptr),
	numPoints_(0),
	allocedSize_(0)
{

}

// allocate for n points
XWinding::XWinding(const size_t n) :
	pPoints_(nullptr),
	numPoints_(0),
	allocedSize_(0)
{
	EnsureAlloced(n);
}


// winding from points
XWinding::XWinding(const Vec3f* verts, const size_t numVerts) :
pPoints_(nullptr),
numPoints_(0),
allocedSize_(0)
{
	size_t i;

	if (!EnsureAlloced(numVerts)) {
		numPoints_ = 0;
		return;
	}
	for (i = 0; i < numVerts; i++) {
		pPoints_[i] = Vec5f(verts[i]);
	}
	numPoints_ = safe_static_cast<int32_t, size_t>(numVerts);
}


// winding from points
XWinding::XWinding(const Vec5f* verts, const size_t numVerts) :
	pPoints_(nullptr),
	numPoints_(0),
	allocedSize_(0)
{
	size_t i;

	if (!EnsureAlloced(numVerts)) {
		numPoints_ = 0;
		return;
	}
	for (i = 0; i < numVerts; i++) {
		pPoints_[i] = verts[i];
	}
	numPoints_ = safe_static_cast<int32_t, size_t>(numVerts);
}

// base winding for plane
XWinding::XWinding(const Vec3f& normal, const float dist) :
	pPoints_(nullptr),
	numPoints_(0),
	allocedSize_(0)
{
	baseForPlane(normal, dist);
}

// base winding for plane
XWinding::XWinding(const Planef& plane) :
	pPoints_(nullptr),
	numPoints_(0),
	allocedSize_(0)
{
	baseForPlane(plane);
}


XWinding::XWinding(const XWinding& winding) :
	pPoints_(nullptr),
	numPoints_(0),
	allocedSize_(0)
{
	size_t i;
	if (!EnsureAlloced(winding.getNumPoints())) {
		numPoints_ = 0;
		return;
	}
	for (i = 0; i < winding.getNumPoints(); i++) {
		pPoints_[i] = winding[i];
	}
	numPoints_ = safe_static_cast<int32_t, size_t>(winding.getNumPoints());
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
	for (i = 0; i < numPoints_; i++) {
		delta = pPoints_[(i + 1) % numPoints_].asVec3() - pPoints_[i].asVec3();
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

	for (i = 0; i < numPoints_; i++) {
		for (j = 0; j < 3; j++) {
			if (pPoints_[i][j] <= MIN_WORLD_COORD || pPoints_[i][j] >= MAX_WORLD_COORD) {
				return true;
			}
		}
	}
	return false;
}


void XWinding::clear(void)
{
	numPoints_ = 0;
	allocedSize_ = 0;
//	delete[] pPoints_;
	X_DELETE_ARRAY(pPoints_, gEnv->pArena);
	pPoints_ = nullptr;
}


void XWinding::print(void) const
{
	// print the bounds.
	AABB bounds;
	GetAABB(bounds);

	const Vec3f& min = bounds.min;
	const Vec3f& max = bounds.max;
	X_LOG0("Winding", "Printing values:");
	X_LOG_BULLET;
	X_LOG0("Winding", "min: (^8%g,%g,%g^7)", min[0], min[1], min[2]);
	X_LOG0("Winding", "max: (^8%g,%g,%g^7)", max[0], max[1], max[2]);
	X_LOG0("Winding", "NumPoints: ^8%i", numPoints_);
	for (int i = 0; i < numPoints_; i++)
	{
		const Vec5f& pl = pPoints_[i];
		X_LOG0("Winding", "P(^8%i^7): (^8%g,%g,%g^7) (^8%g,%g^7)", i,
			pl[0], pl[1], pl[2], pl[3], pl[4]);
	}
}

// ----------------------------------------------------------------------


void XWinding::baseForPlane(const Vec3f& normal, const float dist)
{
	Vec3f org, vright, vup;

	org = normal * dist;

	NormalVectors(normal, vup, vright);

	vup *= static_cast<float>(MAX_WORLD_SIZE);
	vright *= static_cast<float>(MAX_WORLD_SIZE);

	EnsureAlloced(4);
	numPoints_ = 4;
	pPoints_[0] = org - vright + vup;
	pPoints_[1] = org + vright + vup;
	pPoints_[2] = org + vright - vup;
	pPoints_[3] = org - vright - vup;

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
	for (i = 2; i < numPoints_; i++) 
	{
		d1 = pPoints_[i - 1].asVec3() - pPoints_[0].asVec3();
		d2 = pPoints_[i].asVec3() - pPoints_[0].asVec3();
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
	for (i = 0; i < numPoints_; i++) {
		center += pPoints_[i].asVec3();
	}
	center *= (1.0f / numPoints_);
	return center;
}

float XWinding::getRadius(const Vec3f& center) const
{
	int i;
	float radius, r;
	Vec3f dir;

	radius = 0.0f;
	for (i = 0; i < numPoints_; i++) {
		dir = pPoints_[i].asVec3() - center;
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

	if (numPoints_ < 3) {
		normal = Vec3f::zero();
		dist = 0.0f;
		return;
	}

	center = getCenter();
	v1 = pPoints_[0].asVec3() - center;
	v2 = pPoints_[1].asVec3() - center;
	normal = v2.cross(v1);
	normal.normalize();
	dist = pPoints_[0].asVec3() * normal;
}

void XWinding::getPlane(Planef& plane) const
{
	Vec3f v1, v2;
	Vec3f center;

	if (numPoints_ < 3) {
		//	plane.set(Vec3f::zero(), 0);
		core::zero_object(plane);
		return;
	}

	center = getCenter();
	v1 = pPoints_[0].asVec3() - center;
	v2 = pPoints_[1].asVec3() - center;
	plane.setNormal(v2.cross(v1).normalized());
	plane.setDistance(plane.getNormal() * pPoints_[0].asVec3());
}

void XWinding::GetAABB(AABB& box) const
{
	int i;

	if (!numPoints_) {
		box.clear();
		return;
	}

	box.min = box.max = pPoints_[0].asVec3();

	for (i = 1; i < numPoints_; i++) 
	{
		if (pPoints_[i].x < box.min.x) {
			box.min.x = pPoints_[i].x;
		}
		else if (pPoints_[i].x > box.max.x) {
			box.max.x = pPoints_[i].x;
		}

		if (pPoints_[i].y < box.min.y) {
			box.min.y = pPoints_[i].y;
		}
		else if (pPoints_[i].y > box.max.y) {
			box.max.y = pPoints_[i].y;
		}

		if (pPoints_[i].z < box.min.z) {
			box.min.z = pPoints_[i].z;
		}
		else if (pPoints_[i].z > box.max.z) {
			box.max.z = pPoints_[i].z;
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
	for (i = 0; i < numPoints_; i++) 
	{
		d = plane.distance(pPoints_[i].asVec3());
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
	for (i = 0; i < numPoints_; i++)
	{
		d = plane.distance(pPoints_[i].asVec3());
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
	Vec5f*		newPoints;
	int			newNumPoints;
	int			counts[3];
	float		dot;
	int			i, j;
	Vec5f*		p1, *p2;
	Vec5f		mid;
	int			maxpts;


	dists = static_cast<float*>(_alloca((numPoints_ + 4) * sizeof(float)));
	sides = static_cast<uint8_t*>(_alloca((numPoints_ + 4) * sizeof(uint8_t)));

	core::zero_object(counts);

	// determine sides for each point
	for (i = 0; i < numPoints_; i++)
	{
		dists[i] = dot = plane.distance(pPoints_[i].asVec3());
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
		return true;
	}
	// if nothing at the front of the clipping plane
	if (!counts[PlaneSide::FRONT]) {
		numPoints_ = 0;
		return false;
	}
	// if nothing at the back of the clipping plane
	if (!counts[PlaneSide::BACK]) {
		return true;
	}

	maxpts = numPoints_ + 4;		// cant use counts[0]+2 because of fp grouping errors

	newPoints = reinterpret_cast<Vec5f*>(alloca16(maxpts * sizeof(Vec5f)));
	newNumPoints = 0;

	for (i = 0; i < numPoints_; i++) 
	{
		p1 = &pPoints_[i];

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
		p2 = &pPoints_[(i + 1) % numPoints_];

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

		mid.s = p1->s + dot * (p2->s - p1->s);
		mid.t = p1->t + dot * (p2->t - p1->t);

		newPoints[newNumPoints] = mid;
		newNumPoints++;
	}

	if (!EnsureAlloced(newNumPoints, false)) {
		return false;
	}

	numPoints_ = newNumPoints;
	memcpy(pPoints_, newPoints, newNumPoints * sizeof(Vec5f));
	return true;
}


bool XWinding::clip(const Planef &plane, const float epsilon, const bool keepOn)
{
	Vec5f*		newPoints;
	int			newNumPoints;
	int			counts[3];
	float		dot;
	int			i, j;
	Vec5f*	p1, *p2;
	Vec5f		mid;
	int			maxpts;

	X_ASSERT_NOT_NULL(this);

	float	dists[MAX_POINTS_ON_WINDING + 4];
	int		sides[MAX_POINTS_ON_WINDING + 4];

	core::zero_object(counts);


	// determine sides for each point
	for (i = 0; i < numPoints_; i++) {
		dists[i] = dot = plane.distance(pPoints_[i].asVec3());
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
		return true;
	}
	// if nothing at the front of the clipping plane
	if (!counts[PlaneSide::FRONT]) {
		this->clear();
		return false;
	}
	// if nothing at the back of the clipping plane
	if (!counts[PlaneSide::BACK]) {
		return true;
	}

	maxpts = numPoints_ + 4;		// cant use counts[0]+2 because of fp grouping errors

	newPoints = reinterpret_cast<Vec5f*>(alloca16(maxpts * sizeof(Vec5f)));
	newNumPoints = 0;

	for (i = 0; i < numPoints_; i++) {
		p1 = &pPoints_[i];

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
		p2 = &pPoints_[(i + 1) % numPoints_];

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

		mid.s = p1->s + dot * (p2->s - p1->s);
		mid.t = p1->t + dot * (p2->t - p1->t);

		newPoints[newNumPoints] = mid;
		newNumPoints++;
	}

	if (!EnsureAlloced(newNumPoints, false)) {
		return false;
	}

	numPoints_ = newNumPoints;
	memcpy(pPoints_, newPoints, newNumPoints * sizeof(Vec5f));
	return true;
}


XWinding* XWinding::Copy(core::MemoryArenaBase* arena) const
{
	X_ASSERT_NOT_NULL(arena);

	return X_NEW(XWinding, arena, "WindingCopy")(*this);
}


XWinding* XWinding::ReverseWinding(core::MemoryArenaBase* arena) const
{
	X_ASSERT_NOT_NULL(arena);

	XWinding* c = X_NEW(XWinding, arena, "ReverseWinding");
	c->EnsureAlloced(numPoints_);

	for (int i = 0; i < numPoints_; i++) {
		c->pPoints_[i] = pPoints_[numPoints_ - 1 - i];
	}
	c->numPoints_ = numPoints_;
	return c;
}


int XWinding::Split(const Planef& plane, const float epsilon, 
	XWinding **front, XWinding **back, core::MemoryArenaBase* arena) const
{
	X_ASSERT_NOT_NULL(arena);

	int				counts[3];
	float			dot;
	int				i, j;
	const Vec5f *	p1, *p2;
	Vec5f			mid;
	XWinding *		f, *b;
	int				maxpts;

	float	dists[MAX_POINTS_ON_WINDING + 4];
	int		sides[MAX_POINTS_ON_WINDING + 4];

	counts[0] = counts[1] = counts[2] = 0;

	// determine sides for each point
	for (i = 0; i < numPoints_; i++) {
		dists[i] = dot = plane.distance(pPoints_[i].asVec3());

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

	*front = *back = nullptr;

	// if coplanar, put on the front side if the normals match
	if (!counts[PlaneSide::FRONT] && !counts[PlaneSide::BACK]) {
		Planef windingPlane;

		getPlane(windingPlane);
		if (windingPlane.getNormal() * plane.getNormal() > 0.0f) {
			*front = Copy(arena);
			return PlaneSide::FRONT;
		}
		else {
			*back = Copy(arena);
			return PlaneSide::BACK;
		}
	}
	// if nothing at the front of the clipping plane
	if (!counts[PlaneSide::FRONT]) {
		*back = Copy(arena);
		return PlaneSide::BACK;
	}
	// if nothing at the back of the clipping plane
	if (!counts[PlaneSide::BACK]) {
		*front = Copy(arena);
		return PlaneSide::FRONT;
	}

	maxpts = numPoints_ + 4;	// cant use counts[0]+2 because of fp grouping errors

	*front = f = X_NEW(XWinding, arena, "fronWinding")(maxpts);
	*back = b = X_NEW(XWinding, arena, "BackWinding")(maxpts);

	for (i = 0; i < numPoints_; i++) {
		p1 = &pPoints_[i];

		if (sides[i] == PlaneSide::ON) {
			f->pPoints_[f->numPoints_] = *p1;
			f->numPoints_++;
			b->pPoints_[b->numPoints_] = *p1;
			b->numPoints_++;
			continue;
		}

		if (sides[i] == PlaneSide::FRONT) {
			f->pPoints_[f->numPoints_] = *p1;
			f->numPoints_++;
		}

		if (sides[i] == PlaneSide::BACK) {
			b->pPoints_[b->numPoints_] = *p1;
			b->numPoints_++;
		}

		if (sides[i + 1] == PlaneSide::ON || sides[i + 1] == sides[i]) {
			continue;
		}

		// generate a split point
		p2 = &pPoints_[(i + 1) % numPoints_];

		// always calculate the split going from the same side
		// or minor epsilon issues can happen
		if (sides[i] == PlaneSide::FRONT) {
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

			mid.s = p1->s + dot * (p2->s - p1->s);
			mid.t = p1->t + dot * (p2->t - p1->t);
		}
		else {
			dot = dists[i + 1] / (dists[i + 1] - dists[i]);
			for (j = 0; j < 3; j++) {
				// avoid round off error when possible
				if (plane.getNormal()[j] == 1.0f) {
					mid[j] = plane.getDistance();
				}
				else if (plane.getNormal()[j] == -1.0f) {
					mid[j] = -plane.getDistance();
				}
				else {
					mid[j] = (*p2)[j] + dot * ((*p1)[j] - (*p2)[j]);
				}
			}

			mid.s = p2->s + dot * (p1->s - p2->s);
			mid.t = p2->t + dot * (p1->t - p2->t);
		}

		f->pPoints_[f->numPoints_] = mid;
		f->numPoints_++;
		b->pPoints_[b->numPoints_] = mid;
		b->numPoints_++;
	}

	if (f->numPoints_ > maxpts || b->numPoints_ > maxpts) {
		X_WARNING("XWinding", "points exceeded estimate");
	}

	return PlaneSide::CROSS;
}




void XWinding::AddToConvexHull(const XWinding *winding, const Vec3f &normal, const float epsilon)
{
	int				i, j, k;
	Vec3f			dir;
	float			d;
	int				maxPts;
	Vec3f *			hullDirs;
	bool *			hullSide;
	bool			outside;
	int				numNewHullPoints;
	Vec5f *			newHullPoints;

	if (!winding) {
		return;
	}

	maxPts = this->numPoints_ + winding->numPoints_;

	if (!this->EnsureAlloced(maxPts, true)) {
		return;
	}

	newHullPoints = (Vec5f *)alloca16(maxPts * sizeof(Vec5f));
	hullDirs = (Vec3f *)alloca16(maxPts * sizeof(Vec3f));
	hullSide = (bool *)alloca16(maxPts * sizeof(bool));

	for (i = 0; i < winding->numPoints_; i++)
	{
		const Vec5f &p1 = winding->pPoints_[i];

		// calculate hull edge vectors
		for (j = 0; j < this->numPoints_; j++) {
			dir = this->pPoints_[(j + 1) % this->numPoints_].asVec3() - this->pPoints_[j].asVec3();
			dir.normalize();
			hullDirs[j] = normal.cross(dir);
		}

		// calculate side for each hull edge
		outside = false;
		for (j = 0; j < this->numPoints_; j++) {
			dir = p1.asVec3() - this->pPoints_[j].asVec3();
			d = dir * hullDirs[j];
			if (d >= epsilon) {
				outside = true;
			}
			if (d >= -epsilon) {
				hullSide[j] = true;
			}
			else {
				hullSide[j] = false;
			}
		}

		// if the point is effectively inside, do nothing
		if (!outside) {
			continue;
		}

		// find the back side to front side transition
		for (j = 0; j < this->numPoints_; j++) {
			if (!hullSide[j] && hullSide[(j + 1) % this->numPoints_]) {
				break;
			}
		}
		if (j >= this->numPoints_) {
			continue;
		}

		// insert the point here
		newHullPoints[0] = p1;
		numNewHullPoints = 1;

		// copy over all points that aren't double fronts
		j = (j + 1) % this->numPoints_;
		for (k = 0; k < this->numPoints_; k++) {
			if (hullSide[(j + k) % this->numPoints_] && hullSide[(j + k + 1) % this->numPoints_]) {
				continue;
			}
			newHullPoints[numNewHullPoints] = this->pPoints_[(j + k + 1) % this->numPoints_];
			numNewHullPoints++;
		}

		this->numPoints_ = numNewHullPoints;
		memcpy(this->pPoints_, newHullPoints, numNewHullPoints * sizeof(Vec5f));
	}

}


void XWinding::AddToConvexHull(const Vec3f &point, const Vec3f &normal, const float epsilon) 
{
	int				j, k, numHullPoints;
	Vec3f			dir;
	float			d;
	Vec3f *			hullDirs;
	bool *			hullSide;
	Vec5f *			hullPoints;
	bool			outside;

	switch (numPoints_)
	{
		case 0: {
					pPoints_[0] = point;
					numPoints_++;
					return;
		}
		case 1: {
					// don't add the same point second
					if (pPoints_[0].asVec3().compare(point, epsilon)) {
						return;
					}
					pPoints_[1].asVec3() = point;
					numPoints_++;
					return;
		}
		case 2: {
					// don't add a point if it already exists
					if (pPoints_[0].asVec3().compare(point, epsilon) || pPoints_[1].asVec3().compare(point, epsilon)) {
						return;
					}
					// if only two points make sure we have the right ordering according to the normal
					dir = point - pPoints_[0].asVec3();
					dir = dir.cross(pPoints_[1].asVec3() - pPoints_[0].asVec3());
					if (dir[0] == 0.0f && dir[1] == 0.0f && dir[2] == 0.0f) {
						// points don't make a plane
						return;
					}
					if (dir * normal > 0.0f) {
						pPoints_[2].asVec3() = point;
					}
					else {
						pPoints_[2] = pPoints_[1];
						pPoints_[1].asVec3() = point;
					}
					numPoints_++;
					return;
		}
	}

	hullDirs = (Vec3f *)alloca16(numPoints_ * sizeof(Vec3f));
	hullSide = (bool *)alloca16(numPoints_ * sizeof(bool));

	// calculate hull edge vectors
	for (j = 0; j < numPoints_; j++) {
		dir = pPoints_[(j + 1) % numPoints_].asVec3() - pPoints_[j].asVec3();
		hullDirs[j] = normal.cross(dir);
	}

	// calculate side for each hull edge
	outside = false;
	for (j = 0; j < numPoints_; j++) {
		dir = point - pPoints_[j].asVec3();
		d = dir * hullDirs[j];
		if (d >= epsilon) {
			outside = true;
		}
		if (d >= -epsilon) {
			hullSide[j] = true;
		}
		else {
			hullSide[j] = false;
		}
	}

	// if the point is effectively inside, do nothing
	if (!outside) {
		return;
	}

	// find the back side to front side transition
	for (j = 0; j < numPoints_; j++) {
		if (!hullSide[j] && hullSide[(j + 1) % numPoints_]) {
			break;
		}
	}
	if (j >= numPoints_) {
		return;
	}

	hullPoints = reinterpret_cast<Vec5f*>(alloca16((numPoints_ + 1) * sizeof(Vec5f)));

	// insert the point here
	hullPoints[0] = point;
	numHullPoints = 1;

	// copy over all points that aren't double fronts
	j = (j + 1) % numPoints_;
	for (k = 0; k < numPoints_; k++) {
		if (hullSide[(j + k) % numPoints_] && hullSide[(j + k + 1) % numPoints_]) {
			continue;
		}
		hullPoints[numHullPoints] = pPoints_[(j + k + 1) % numPoints_];
		numHullPoints++;
	}

	if (!EnsureAlloced(numHullPoints, false)) {
		return;
	}
	numPoints_ = numHullPoints;
	memcpy(pPoints_, hullPoints, numHullPoints * sizeof(Vec3f));
}

// ----------------------------------------------------------------------

float XWinding::TriangleArea(const Vec3f& a, const Vec3f& b, const Vec3f& c)
{
	Vec3f v1, v2;
	Vec3f cross;

	v1 = b - a;
	v2 = c - a;
	cross = v1.cross(v2);
	return 0.5f * cross.length();
}

// ----------------------------------------------------------------------


// ISerialize
bool XWinding::SSave(core::XFile* pFile) const
{
	X_ASSERT_NOT_NULL(pFile);

	uint32_t i, num = safe_static_cast<uint32_t, size_t>(numPoints_);

	if (!pFile->writeObj(num)) {
		X_ERROR("Winding", "Failed to save winding to file");
		return false;
	}

	for (i = 0; i < num; i++)
	{
		// save each point.
		const Vec5f& point = pPoints_[i];

		if (!pFile->writeObj(point)) {
			X_ERROR("Winding", "Failed to save winding to file");
			return false;
		}
	}

	return true;
}

bool XWinding::SLoad(core::XFile* pFile)
{
	X_ASSERT_NOT_NULL(pFile);

	uint32_t i, num;

	if (!pFile->readObj(num)) {
		X_ERROR("Winding", "Failed to load winding from file");
		return false;
	}

	if (!EnsureAlloced(num, false)) {
		X_ERROR("Winding", "Failed to allocate %i points for file winding", num);
		return false;
	}

	for (i = 0; i < num; i++)
	{
		// save each point.
		Vec5f& point = pPoints_[i];

		if (!pFile->readObj(point)) {
			X_ERROR("Winding", "Failed to load winding point from file");
			this->clear();
			return false;
		}
	}

	numPoints_ = num;
	return true;
}
// ~ISerialize


