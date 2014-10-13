

X_INLINE AABB::AABB()
{

}

X_INLINE AABB::AABB(float radius)
{
	min = Vec3f(radius);
	max = -min;
}

X_INLINE AABB::AABB(const Vec3f& center, float radius)
{
	Vec3f rad(radius);
	min = center - rad;
	max = center + rad;
}

X_INLINE AABB::AABB(const Vec3f &min_, const Vec3f &max_) : min(min_), max(max_)
{
}

X_INLINE AABB::AABB(const AABB &oth) : min(oth.min), max(oth.max)
{
}


X_INLINE void AABB::set(float radius)
{
	min = Vec3f(radius);
	max = -min;
}

X_INLINE void AABB::set(const Vec3f& center, float radius)
{
	Vec3f rad(radius);
	min = center - rad;
	max = center + rad;
}

X_INLINE void AABB::set(const Vec3f &min, const Vec3f &max)
{
	this->min = min;
	this->max = max;
}

X_INLINE void AABB::set(const AABB &oth)
{
	max = oth.max;
	min = oth.min;
}


X_INLINE void AABB::clear()
{
	max[0] = max[1] = max[2] = -INFINITY;
	min[0] = min[1] = min[2] = INFINITY;
}


X_INLINE bool AABB::isEmpty() const
{
	return min == max;
}

 // retruns if the box contains anyspace 
X_INLINE Vec3f AABB::center() const
{
	// size / 2
	return (min + max)*0.5f;
}
	// the center point of the box
X_INLINE Vec3f AABB::size() const
{
	return (max - min);
}
	// the size of the box.
X_INLINE float AABB::radius() const
{
	return min.distance(max) * 0.5f; // 1/2 it for radius
}

X_INLINE float AABB::radiusSqr() const
{
	return ((max - min)*0.5f).lengthSquared();
}

X_INLINE float AABB::volume() const
{
	return (min.x - max.x) * (min.y - max.y) * (min.z - max.z);
}


// add to the bounding box.
X_INLINE void AABB::add(const Vec3f &v)
{
	// SHieeeeeeeeeetttt!
	min.checkMin(v);
	max.checkMax(v);
}

X_INLINE void AABB::add(const Vec3f& v, float radius)
{
	Vec3f rad(radius);
	min.checkMin(v-rad);
	max.checkMax(v+rad);
}

X_INLINE void AABB::add(const AABB& bb)
{
	min.checkMin(bb.min);
	max.checkMax(bb.max);
}


// --------------------------------------------


// shake that booty box
X_INLINE void AABB::move(const Vec3f& v)
{
	min += v;
	max += v;
}

// expands in both directions
X_INLINE void AABB::expand(const Vec3f& v)
{
	min -= v;
	max += v;
}

// clips the current BB with the provided BB
X_INLINE void AABB::clip(const AABB& bb)
{
	min.checkMax(bb.min);
	max.checkMin(bb.max);
}

// Check if this bounding box overlap with bounding box of sphere.
X_INLINE bool AABB::containsPoint(const Vec3f& pos) const
{
	return min <= pos && max >= pos;
}

// check if a BB fits inside this BB.
X_INLINE bool AABB::containsBox(const AABB& b) const
{
	return min <= b.min && max >= b.max;
}

X_INLINE bool AABB::containsSphere(const Sphere& b) const
{
	// we just treat it like a box.
	Vec3f rad(b.radius());
	return (b.center() + rad) <= max &&
		(b.center() - rad) >= min;
}

X_INLINE bool AABB::containsSphere(const Vec3f &aCenter, float aRadius) const
{
	Vec3f rad(aRadius);
	return (aCenter + rad) <= max &&
		(aCenter - rad) >= min;
}


// checks if the BB's intersect.
X_INLINE bool AABB::intersects(const AABB& b) const
{
	// any point of B insdie this box?
	return  b.min <= max && b.max >= min;
}

X_INLINE float AABB::distance(const Vec3f& v) const
{
	Vec3f Near(v); // windows defines 'near' -_-
	Near.checkMax(min);
	Near.checkMin(max);
	return Near.distance(v);
}

X_INLINE float AABB::distanceSqr(const Vec3f& v) const
{
	Vec3f Near(v);
	Near.checkMax(min);
	Near.checkMin(max);
	return Near.distanceSquared(v);
}
