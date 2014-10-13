#include "stdafx.h"
#include "Winding.h"


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
	n = (n + 3) & ~3;	// align up to multiple of four
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

// -----------------------------
#define IEEE_FLT_MANTISSA_BITS		23
#define IEEE_FLT_EXPONENT_BITS		8
#define IEEE_FLT_EXPONENT_BIAS		127
#define IEEE_FLT_SIGN_BIT           31


const int SMALLEST_NON_DENORMAL = 1 << IEEE_FLT_MANTISSA_BITS;
const int NAN_VALUE = 0x7f800000;
const float FLT_SMALLEST_NON_DENORMAL = *reinterpret_cast< const float * >(&SMALLEST_NON_DENORMAL);


X_DISABLE_WARNING(4756)

float InvSqrt(float x) 
{
	return (x > FLT_SMALLEST_NON_DENORMAL) ? sqrtf(1.0f / x) : INFINITY;
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

#if 0
#else

void XWinding::BaseForPlane(const Vec3f &normal, const float dist)
{
	Vec3f org, vright, vup;

	org = normal * dist;

	NormalVectors(normal, vup, vright);

	vup *= bsp::MAX_WORLD_SIZE;
	vright *= bsp::MAX_WORLD_SIZE;

	EnsureAlloced(4);
	numPoints = 4;
	p[0] = org - vright + vup;
	p[1] = org + vright + vup;
	p[2] = org + vright - vup;
	p[3] = org - vright - vup;


//	Vec3f temp[4];
//	memcpy(temp, p, sizeof(temp));
}

#endif 

void XWinding::BaseForPlane(const Planef &plane)
{
	BaseForPlane(plane.getNormal(), plane.getDistance());
}


float XWinding::GetArea(void) const
{
	int i;
	Vec3f d1, d2, cross;
	float total;

	total = 0.0f;
	for (i = 2; i < numPoints; i++) {
		d1 = p[i - 1] - p[0];
		d2 = p[i] - p[0];
		cross = d1.cross(d2);
		total += cross.length();
	}
	return total * 0.5f;
}

Vec3f XWinding::GetCenter(void) const
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

float XWinding::GetRadius(const Vec3f &center) const
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

void XWinding::GetPlane(Vec3f &normal, float &dist) const
{
	Vec3f v1, v2, center;

	if (numPoints < 3) {
		normal = Vec3f::zero();
		dist = 0.0f;
		return;
	}

	center = GetCenter();
	v1 = p[0] - center;
	v2 = p[1] - center;
	normal = v2.cross(v1);
	normal.normalize();
	dist = p[0] * normal;
}


void XWinding::GetPlane(Planef &plane) const 
{
	Vec3f v1, v2;
	Vec3f center;

	if (numPoints < 3) {
		//	plane.set(Vec3f::zero(), 0);
		core::zero_object(plane);
		return;
	}

	center = GetCenter();
	v1 = p[0] - center;
	v2 = p[1] - center;
	plane.setNormal(v2.cross(v1));
	// plane.Normal_.normalize();
	//	plane.normalize();
	//	plane.FitThroughPoint(p[0]);
	plane.setDistance(plane.getNormal() * p[0]);
}

float XWinding::PlaneDistance(const Planef &plane) const
{
	/*	int		i;
	float	d, min, max;

	min = idMath::INFINITY;
	max = -min;
	for (i = 0; i < numPoints; i++) {
	d = plane.distance(p[i]);
	if (d < min) {
	min = d;
	if (FLOATSIGNBITSET(min) & FLOATSIGNBITNOTSET(max)) {
	return 0.0f;
	}
	}
	if (d > max) {
	max = d;
	if (FLOATSIGNBITSET(min) & FLOATSIGNBITNOTSET(max)) {
	return 0.0f;
	}
	}
	}
	if (FLOATSIGNBITNOTSET(min)) {
	return min;
	}
	if (FLOATSIGNBITSET(max)) {
	return max;
	}*/
	return 0.0f;
}

Planeside::Enum XWinding::PlaneSide(const Planef &plane, const float epsilon) const
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
				return Planeside::CROSS;
			}
			back = true;
			continue;
		}
		else if (d > epsilon) {
			if (back) {
				return Planeside::CROSS;
			}
			front = true;
			continue;
		}
	}

	if (back) {
		return Planeside::BACK;
	}
	if (front) {
		return Planeside::FRONT;
	}
	return Planeside::ON;
}



// -----------------------------


int XWinding::Split(const Planef& plane, const float epsilon, XWinding **front, XWinding **back) const
{
	int				counts[3];
	float			dot;
	int				i, j;
	const Vec3f *	p1, *p2;
	Vec3f			mid;
	XWinding *		f, *b;
	int				maxpts;

	float	dists[MAX_POINTS_ON_WINDING + 4];
	int		sides[MAX_POINTS_ON_WINDING + 4];

	counts[0] = counts[1] = counts[2] = 0;

	// determine sides for each point
	for (i = 0; i < numPoints; i++) {
		dists[i] = dot = plane.distance(p[i]);

		if (dot > epsilon) {
			sides[i] = Planeside::FRONT;
		}
		else if (dot < -epsilon) {
			sides[i] =  Planeside::BACK;
		}
		else {
			sides[i] = Planeside::ON;
		}
		counts[sides[i]]++;
	}
	sides[i] = sides[0];
	dists[i] = dists[0];

	*front = *back = NULL;

	// if coplanar, put on the front side if the normals match
	if (!counts[Planeside::FRONT] && !counts[Planeside::BACK]) {
		Planef windingPlane;

		GetPlane(windingPlane);
		if (windingPlane.getNormal() * plane.getNormal() > 0.0f) {
			*front = Copy();
			return Planeside::FRONT;
		}
		else {
			*back = Copy();
			return Planeside::BACK;
		}
	}
	// if nothing at the front of the clipping plane
	if (!counts[Planeside::FRONT]) {
		*back = Copy();
		return Planeside::BACK;
	}
	// if nothing at the back of the clipping plane
	if (!counts[Planeside::BACK]) {
		*front = Copy();
		return Planeside::FRONT;
	}

	maxpts = numPoints + 4;	// cant use counts[0]+2 because of fp grouping errors

	*front = f = new XWinding(maxpts);
	*back = b = new XWinding(maxpts);

	for (i = 0; i < numPoints; i++) {
		p1 = &p[i];

		if (sides[i] == Planeside::ON) {
			f->p[f->numPoints] = *p1;
			f->numPoints++;
			b->p[b->numPoints] = *p1;
			b->numPoints++;
			continue;
		}

		if (sides[i] == Planeside::FRONT) {
			f->p[f->numPoints] = *p1;
			f->numPoints++;
		}

		if (sides[i] == Planeside::BACK) {
			b->p[b->numPoints] = *p1;
			b->numPoints++;
		}

		if (sides[i + 1] == Planeside::ON || sides[i + 1] == sides[i]) {
			continue;
		}

		// generate a split point
		p2 = &p[(i + 1) % numPoints];

		// always calculate the split going from the same side
		// or minor epsilon issues can happen
		if (sides[i] == Planeside::FRONT) {
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
		}

		f->p[f->numPoints] = mid;
		f->numPoints++;
		b->p[b->numPoints] = mid;
		b->numPoints++;
	}

	if (f->numPoints > maxpts || b->numPoints > maxpts) {
		//	idLib::common->FatalError("idWinding::Split: points exceeded estimate.");
		X_WARNING("XWinding", "points exceeded estimate");
	}

	return Planeside::CROSS;
}

#if 0
#else

XWinding* XWinding::Clip(const Planef &plane, const float epsilon, const bool keepOn)
{
	Vec3f*		newPoints;
	int			newNumPoints;
	int			counts[3];
	float		dot;
	int			i, j;
	Vec3f *	p1, *p2;
	Vec3f		mid;
	int			maxpts;

	X_ASSERT_NOT_NULL(this);

	float	dists[MAX_POINTS_ON_WINDING + 4];
	int		sides[MAX_POINTS_ON_WINDING + 4];

	counts[Planeside::FRONT] = counts[Planeside::BACK] = counts[Planeside::ON] = 0;



	// determine sides for each point
	for (i = 0; i < numPoints; i++) {
		dists[i] = dot = plane.distance(p[i]);
		if (dot > epsilon) {
			sides[i] = Planeside::FRONT;
		}
		else if (dot < -epsilon) {
			sides[i] = Planeside::BACK;
		}
		else {
			sides[i] = Planeside::ON;
		}
		counts[sides[i]]++;
	}
	sides[i] = sides[0];
	dists[i] = dists[0];

	// if the winding is on the plane and we should keep it
	if (keepOn && !counts[Planeside::FRONT] && !counts[Planeside::BACK]) {
		return this;
	}
	// if nothing at the front of the clipping plane
	if (!counts[Planeside::FRONT]) {
		delete this;
		return NULL;
	}
	// if nothing at the back of the clipping plane
	if (!counts[Planeside::BACK]) {
		return this;
	}

	maxpts = numPoints + 4;		// cant use counts[0]+2 because of fp grouping errors

	newPoints = (Vec3f *)_alloca16(maxpts * sizeof(Vec3f));
	newNumPoints = 0;

	for (i = 0; i < numPoints; i++) {
		p1 = &p[i];

		if (newNumPoints + 1 > maxpts) {
			return this;		// can't split -- fall back to original
		}

		if (sides[i] == Planeside::ON) {
			newPoints[newNumPoints] = *p1;
			newNumPoints++;
			continue;
		}

		if (sides[i] == Planeside::FRONT) {
			newPoints[newNumPoints] = *p1;
			newNumPoints++;
		}

		if (sides[i + 1] == Planeside::ON || sides[i + 1] == sides[i]) {
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

#endif


XWinding *XWinding::Copy(void) const
{
	XWinding *w;

	w = new XWinding(numPoints);
	w->numPoints = numPoints;
	memcpy(w->p, p, numPoints * sizeof(p[0]));
	return w;
}





/*
=============
Adds the given winding to the convex hull.
Assumes the current winding already is a convex hull with three or more points.
=============
*/
void XWinding::AddToConvexHull(const XWinding *winding, const Vec3f &normal, const float epsilon) {
	int				i, j, k;
	Vec3f			dir;
	float			d;
	int				maxPts;
	Vec3f *			hullDirs;
	bool *			hullSide;
	bool			outside;
	int				numNewHullPoints;
	Vec3f *			newHullPoints;

	if (!winding) {
		return;
	}

	maxPts = this->numPoints + winding->numPoints;

	if (!this->EnsureAlloced(maxPts, true)) {
		return;
	}

	newHullPoints = (Vec3f *)_alloca(maxPts * sizeof(Vec3f));
	hullDirs = (Vec3f *)_alloca(maxPts * sizeof(Vec3f));
	hullSide = (bool *)_alloca(maxPts * sizeof(bool));

	for (i = 0; i < winding->numPoints; i++) {
		const Vec3f &p1 = winding->p[i];

		// calculate hull edge vectors
		for (j = 0; j < this->numPoints; j++) {
			dir = this->p[(j + 1) % this->numPoints] - this->p[j];
			dir.normalize();
			hullDirs[j] = normal.cross(dir);
		}

		// calculate side for each hull edge
		outside = false;
		for (j = 0; j < this->numPoints; j++) {
			dir = p1 - this->p[j];
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
		for (j = 0; j < this->numPoints; j++) {
			if (!hullSide[j] && hullSide[(j + 1) % this->numPoints]) {
				break;
			}
		}
		if (j >= this->numPoints) {
			continue;
		}

		// insert the point here
		newHullPoints[0] = p1;
		numNewHullPoints = 1;

		// copy over all points that aren't double fronts
		j = (j + 1) % this->numPoints;
		for (k = 0; k < this->numPoints; k++) {
			if (hullSide[(j + k) % this->numPoints] && hullSide[(j + k + 1) % this->numPoints]) {
				continue;
			}
			newHullPoints[numNewHullPoints] = this->p[(j + k + 1) % this->numPoints];
			numNewHullPoints++;
		}

		this->numPoints = numNewHullPoints;
		memcpy(this->p, newHullPoints, numNewHullPoints * sizeof(Vec3f));
	}
}

/*
=============
idWinding::AddToConvexHull

Add a point to the convex hull.
The current winding must be convex but may be degenerate and can have less than three points.
=============
*/
void XWinding::AddToConvexHull(const Vec3f &point, const Vec3f &normal, const float epsilon) {
	int				j, k, numHullPoints;
	Vec3f			dir;
	float			d;
	Vec3f *			hullDirs;
	bool *			hullSide;
	Vec3f *			hullPoints;
	bool			outside;

	switch (numPoints) {
		case 0: {
					p[0] = point;
					numPoints++;
					return;
		}
		case 1: {
					// don't add the same point second
					if (p[0].compare(point, epsilon)) {
						return;
					}
					p[1] = point;
					numPoints++;
					return;
		}
		case 2: {
					// don't add a point if it already exists
					if (p[0].compare(point, epsilon) || p[1].compare(point, epsilon)) {
						return;
					}
					// if only two points make sure we have the right ordering according to the normal
					dir = point - p[0];
					dir = dir.cross(p[1] - p[0]);
					if (dir[0] == 0.0f && dir[1] == 0.0f && dir[2] == 0.0f) {
						// points don't make a plane
						return;
					}
					if (dir * normal > 0.0f) {
						p[2] = point;
					}
					else {
						p[2] = p[1];
						p[1] = point;
					}
					numPoints++;
					return;
		}
	}

	hullDirs = (Vec3f *)_alloca(numPoints * sizeof(Vec3f));
	hullSide = (bool *)_alloca(numPoints * sizeof(bool));

	// calculate hull edge vectors
	for (j = 0; j < numPoints; j++) {
		dir = p[(j + 1) % numPoints] - p[j];
		hullDirs[j] = normal.cross(dir);
	}

	// calculate side for each hull edge
	outside = false;
	for (j = 0; j < numPoints; j++) {
		dir = point - p[j];
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
	for (j = 0; j < numPoints; j++) {
		if (!hullSide[j] && hullSide[(j + 1) % numPoints]) {
			break;
		}
	}
	if (j >= numPoints) {
		return;
	}

	hullPoints = (Vec3f *)_alloca((numPoints + 1) * sizeof(Vec3f));

	// insert the point here
	hullPoints[0] = point;
	numHullPoints = 1;

	// copy over all points that aren't double fronts
	j = (j + 1) % numPoints;
	for (k = 0; k < numPoints; k++) {
		if (hullSide[(j + k) % numPoints] && hullSide[(j + k + 1) % numPoints]) {
			continue;
		}
		hullPoints[numHullPoints] = p[(j + k + 1) % numPoints];
		numHullPoints++;
	}

	if (!EnsureAlloced(numHullPoints, false)) {
		return;
	}
	numPoints = numHullPoints;
	memcpy(p, hullPoints, numHullPoints * sizeof(Vec3f));
}
