
// constructors are not unit tested, only the set.
// if you change the constructors consider adding to the unit test.
X_INLINE OBB::OBB(Matrix33f m33, const Vec3f& center, const Vec3f& hlv)
{
	set(m33,center,hlv);
}

X_INLINE OBB::OBB(Matrix33f m33, const AABB& aabb)
{
	set(m33, aabb);
}

X_INLINE OBB::OBB(Quatf quat, const AABB& aabb)
{
	set(quat, aabb);
}


X_INLINE void OBB::set(Matrix33f m33, const Vec3f& center, const Vec3f& hlv)
{
	orientation_ = m33;
	this->center_ = orientation_ * center;
	this->halfLVec_ = hlv;
}

X_INLINE void OBB::set(Matrix33f m33, const AABB& aabb)
{
	orientation_ = m33;
	this->center_ = orientation_ * aabb.center();
	this->halfLVec_ = aabb.halfVec();
}

X_INLINE void OBB::set(Quatf quat, const AABB& aabb)
{
	orientation_ = quat.toMatrix33();
	this->center_ = orientation_ * aabb.center();
	this->halfLVec_ = aabb.halfVec();
}

// --------------------------------------

X_INLINE Vec3f OBB::center(void) const
{
	return center_;
}

X_INLINE Vec3f OBB::size(void) const
{
	return halfLVec_ * 2.f;
}

X_INLINE Vec3f OBB::halfVec(void) const
{
	return halfLVec_;
}

X_INLINE const Matrix33f& OBB::orientation(void) const
{
	return orientation_;
}