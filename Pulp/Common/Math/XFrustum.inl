


X_INLINE XFrustum::XFrustum() 
{
	near_ = 0.f;
	far_ = 0.f;
	left_ = 0.f;
	up_ = 0.f;
	invFar_ = 0.f;

	width_ = 0u;
	height_ = 0u;

	fov_ = 0.f;
	projectionRatio_ = 0.f;
	pixelAspectRatio_ = 0.f;

	core::zero_object(idx_);
	core::zero_object(idy_);
	core::zero_object(idz_);
}



// set some data
X_INLINE void XFrustum::setPosition(const Vec3f& pos)
{
	mat_.setTranslate(pos);
	UpdateFrustum();
}

X_INLINE void XFrustum::setAxis(const Matrix33f& mat)
{
	Vec3f temp = mat_.getTranslate();
	mat_ = mat;
	mat_.setTranslate(temp);

	UpdateFrustum();
}

X_INLINE void XFrustum::setSize(float dNear, float dFar, float dLeft, float dUp)
{
	X_UNUSED(dNear);
	X_UNUSED(dFar);
	X_UNUSED(dLeft);
	X_UNUSED(dUp);
	X_ASSERT_NOT_IMPLEMENTED();
}

// get a goat
X_INLINE const Vec3f XFrustum::getPosition(void) const
{
	return mat_.getTranslate();
}

X_INLINE const Matrix33f XFrustum::getAxis(void) const
{
	mat_;
}

X_INLINE const Matrix34f& XFrustum::getMatrix(void) const
{
	return mat_;
}

X_INLINE Vec3f XFrustum::getCenter(void) const
{
	Vec3f pos = (edge_flt_ - edge_nlt_) * 0.5f;
	return Matrix33f(mat_) * pos;
}


X_INLINE bool XFrustum::isValid(void) const
{
	return getFarPlane() > getNearPlane();
}
					

X_INLINE float32_t XFrustum::getNearPlane(void) const
{ 
	return edge_nlt_.y;
}

X_INLINE float32_t XFrustum::getFarPlane(void) const
{ 
	return edge_flt_.y;
}

X_INLINE Vec3f XFrustum::getEdgeP(void) const
{ 
	return edge_plt_; 
}

X_INLINE Vec3f XFrustum::getEdgeN(void) const
{
	return edge_nlt_;
}

X_INLINE Vec3f XFrustum::getEdgeF(void) const
{
	return edge_flt_; 
}

X_INLINE float XFrustum::getLeft(void) const
{
	return left_;
}
					
X_INLINE float XFrustum::getUp(void) const
{
	return up_;
}


// ----------------------------------------------------------------					

X_INLINE bool XFrustum::cullPoint(const Vec3f& point) const
{
	if (planes_[FrustumPlane::NEAR].distance(point) > 0)
		return false;
	if (planes_[FrustumPlane::RIGHT].distance(point)> 0)
		return false;
	if (planes_[FrustumPlane::LEFT].distance(point) > 0)
		return false;
	if (planes_[FrustumPlane::TOP].distance(point) > 0)
		return false;
	if (planes_[FrustumPlane::BOTTOM].distance(point) > 0)
		return false;
	if (planes_[FrustumPlane::FAR].distance(point) > 0)
		return false;

	return true;
}



X_INLINE bool XFrustum::cullAABB_Fast(const AABB& box) const
{
	return cullAABB_FastT(box) == CullType::EXCLUSION;
}

X_INLINE bool XFrustum::cullAABB_Exact(const AABB& box) const
{
	CullType::Enum o = cullAABB_FastT(box);
	if (o == CullType::EXCLUSION)
		return false;
	if (o == CullType::INCLUSION) 
		return true;

	return AdditionalCheck(box) == CullType::EXCLUSION;
}

X_INLINE CullType::Enum XFrustum::cullAABB_FastT(const AABB& box) const
{
	const float* p = &box.min.x;
	uint32 x, y, z;

	x = idx_[0]; y = idy_[0]; z = idz_[0];
	if (planes_[FrustumPlane::NEAR].distance(Vec3f(p[x], p[y], p[z])) > 0)
		return CullType::EXCLUSION;

	x = idx_[1]; y = idy_[1]; z = idz_[1];
	if (planes_[FrustumPlane::FAR].distance(Vec3f(p[x], p[y], p[z])) > 0)
		return CullType::EXCLUSION;

	x = idx_[2]; y = idy_[2]; z = idz_[2];
	if (planes_[FrustumPlane::RIGHT].distance(Vec3f(p[x], p[y], p[z])) > 0)
		return CullType::EXCLUSION;

	x = idx_[3]; y = idy_[3]; z = idz_[3];
	if (planes_[FrustumPlane::LEFT].distance(Vec3f(p[x], p[y], p[z])) > 0)
		return CullType::EXCLUSION;

	x = idx_[4]; y = idy_[4]; z = idz_[4];
	if (planes_[FrustumPlane::TOP].distance(Vec3f(p[x], p[y], p[z])) > 0)
		return CullType::EXCLUSION;

	x = idx_[5]; y = idy_[5]; z = idz_[5];
	if (planes_[FrustumPlane::BOTTOM].distance(Vec3f(p[x], p[y], p[z])) > 0)
		return CullType::EXCLUSION;

	return CullType::OVERLAP;
}

X_INLINE CullType::Enum XFrustum::cullAABB_ExactT(const AABB& box) const
{
	CullType::Enum o = cullAABB_FastT(box);

	if (o == CullType::OVERLAP)
		return AdditionalCheck(box);

	return o;
}

// ---------------------------------------------------------------

X_INLINE bool XFrustum::cullOBB_Fast(const OBB& box) const
{
	return cullOBB_FastT(box) == CullType::EXCLUSION;
}

X_INLINE bool XFrustum::cullOBB_Exact(const OBB& box) const
{
	return cullOBB_ExactT(box) == CullType::EXCLUSION;
}

X_INLINE CullType::Enum XFrustum::cullOBB_FastT(const OBB& obb) const
{
	//transform the obb-center into world-space
	Vec3f p = obb.orientation() * obb.center();

	//extract the orientation-vectors from the columns of the 3x3 matrix
	//and scale them by the half-lengths
	Vec3f ax = obb.orientation().getColumn(0)*obb.halfVec().x;
	Vec3f ay = obb.orientation().getColumn(1)*obb.halfVec().y;
	Vec3f az = obb.orientation().getColumn(2)*obb.halfVec().z;

	//we project the axes of the OBB onto the normal of each of the 6 planes.
	//If the absolute value of the distance from the center of the OBB to the plane 
	//is larger then the "radius" of the OBB, then the OBB is outside the frustum.
	float32_t t;
	if ((t = planes_[FrustumPlane::NEAR].distance(p)) > 0.0f) {
		if (t > (math<float>::abs(planes_[FrustumPlane::NEAR].getNormal() | ax) +
			math<float>::abs(planes_[FrustumPlane::NEAR].getNormal() | ay) +
			math<float>::abs(planes_[FrustumPlane::NEAR].getNormal() | az)))
			return CullType::EXCLUSION;
	}

	if ((t = planes_[FrustumPlane::FAR].distance(p)) > 0.0f) {
		if (t > (math<float>::abs(planes_[FrustumPlane::FAR].getNormal() | ax) +
			math<float>::abs(planes_[FrustumPlane::FAR].getNormal() | ay) +
			math<float>::abs(planes_[FrustumPlane::FAR].getNormal() | az)))
			return CullType::EXCLUSION;
	}

	if ((t = planes_[FrustumPlane::RIGHT].distance(p)) > 0.0f) {
		if (t > (math<float>::abs(planes_[FrustumPlane::RIGHT].getNormal() | ax) +
			math<float>::abs(planes_[FrustumPlane::RIGHT].getNormal() | ay) +
			math<float>::abs(planes_[FrustumPlane::RIGHT].getNormal() | az)))
			return CullType::EXCLUSION;
	}

	if ((t = planes_[FrustumPlane::LEFT].distance(p)) > 0.0f) {
		if (t > (math<float>::abs(planes_[FrustumPlane::LEFT].getNormal() | ax) +
			math<float>::abs(planes_[FrustumPlane::LEFT].getNormal() | ay) +
			math<float>::abs(planes_[FrustumPlane::LEFT].getNormal() | az)))
			return CullType::EXCLUSION;
	}

	if ((t = planes_[FrustumPlane::TOP].distance(p)) > 0.0f) {
		if (t > (math<float>::abs(planes_[FrustumPlane::TOP].getNormal() | ax) +
			math<float>::abs(planes_[FrustumPlane::TOP].getNormal() | ay) +
			math<float>::abs(planes_[FrustumPlane::TOP].getNormal() | az)))
			return CullType::EXCLUSION;
	}

	if ((t = planes_[FrustumPlane::BOTTOM].distance(p)) > 0.0f) {
		if (t > (math<float>::abs(planes_[FrustumPlane::BOTTOM].getNormal() | ax) +
			math<float>::abs(planes_[FrustumPlane::BOTTOM].getNormal() | ay) +
			math<float>::abs(planes_[FrustumPlane::BOTTOM].getNormal() | az)))
			return CullType::EXCLUSION;
	}

	//probably the OBB is visible! 
	//With this test we can't be sure if the OBB partially visible or totally included or  
	//totally outside the frustum but still intersecting one of the 6 planes (=worst case)
	return CullType::OVERLAP;
}

X_INLINE CullType::Enum XFrustum::cullOBB_ExactT(const OBB& obb) const
{
	//transform the obb-center into world-space
	Vec3f p = obb.orientation() * obb.center();

	//extract the orientation-vectors from the columns of the 3x3 matrix
	//and scale them by the half-lengths
	float32_t scale = 1.f;
	Vec3f ax = obb.orientation().getColumn(0)*obb.halfVec().x*scale;
	Vec3f ay = obb.orientation().getColumn(1)*obb.halfVec().y*scale;
	Vec3f az = obb.orientation().getColumn(2)*obb.halfVec().z*scale;

	//we project the axes of the OBB onto the normal of each of the 6 planes.
	//If the absolute value of the distance from the center of the OBB to the plane 
	//is larger then the "radius" of the OBB, then the OBB is outside the frustum.
	float32_t t0, t1, t2, t3, t4, t5;
	bool mt0, mt1, mt2, mt3, mt4, mt5;

	if (mt0 = (t0 = planes_[FrustumPlane::NEAR].distance(p)) > 0.0f) {
		if (t0 > (math<float>::abs(planes_[FrustumPlane::NEAR].getNormal() | ax) +
			math<float>::abs(planes_[FrustumPlane::NEAR].getNormal() | ay) +
			math<float>::abs(planes_[FrustumPlane::NEAR].getNormal() | az)))
			return CullType::EXCLUSION;
	}

	if (mt1 = (t1 = planes_[FrustumPlane::FAR].distance(p)) > 0.0f)	{
		if (t1 > (math<float>::abs(planes_[FrustumPlane::FAR].getNormal() | ax) +
			math<float>::abs(planes_[FrustumPlane::FAR].getNormal() | ay) +
			math<float>::abs(planes_[FrustumPlane::FAR].getNormal() | az)))
			return CullType::EXCLUSION;
	}

	if (mt2 = (t2 = planes_[FrustumPlane::RIGHT].distance(p)) > 0.0f) {
		if (t2 > (math<float>::abs(planes_[FrustumPlane::RIGHT].getNormal() | ax) +
			math<float>::abs(planes_[FrustumPlane::RIGHT].getNormal() | ay) +
			math<float>::abs(planes_[FrustumPlane::RIGHT].getNormal() | az)))
			return CullType::EXCLUSION;
	}

	if (mt3 = (t3 = planes_[FrustumPlane::LEFT].distance(p)) > 0.0f)	{
		if (t3 > (math<float>::abs(planes_[FrustumPlane::LEFT].getNormal() | ax) +
			math<float>::abs(planes_[FrustumPlane::LEFT].getNormal() | ay) +
			math<float>::abs(planes_[FrustumPlane::LEFT].getNormal() | az)))
			return CullType::EXCLUSION;
	}

	if (mt4 = (t4 = planes_[FrustumPlane::TOP].distance(p)) > 0.0f) {
		if (t4 > (math<float>::abs(planes_[FrustumPlane::TOP].getNormal() | ax) +
			math<float>::abs(planes_[FrustumPlane::TOP].getNormal() | ay) +
			math<float>::abs(planes_[FrustumPlane::TOP].getNormal() | az)))
			return CullType::EXCLUSION;
	}

	if (mt5 = (t5 = planes_[FrustumPlane::BOTTOM].distance(p)) > 0.0f) {
		if (t5 > (math<float>::abs(planes_[FrustumPlane::BOTTOM].getNormal() | ax) + 
			math<float>::abs(planes_[FrustumPlane::BOTTOM].getNormal() | ay) + 
			math<float>::abs(planes_[FrustumPlane::BOTTOM].getNormal() | az)))
			return CullType::EXCLUSION;
	}

	//if obb-center is in view-frustum, then stop further calculation
	if (!(mt0 | mt1 | mt2 | mt3 | mt4 | mt5)) 
		return CullType::OVERLAP;

	return AdditionalCheck(obb, scale);
}


// ---------------------------------------------------------------

X_INLINE bool XFrustum::cullSphere_Fast(const Sphere& sphere) const
{
	return cullSphere_FastT(sphere) == CullType::EXCLUSION;
}

X_INLINE bool XFrustum::cullSphere_Exact(const Sphere& sphere) const
{
	return cullSphere_ExactT(sphere) == CullType::EXCLUSION;
}

X_INLINE CullType::Enum XFrustum::cullSphere_FastT(const Sphere& s) const
{
	if ((planes_[FrustumPlane::NEAR].distance( s.center())) > s.radius())
		return CullType::EXCLUSION;

	if ((planes_[FrustumPlane::FAR].distance(s.center())) > s.radius())
		return CullType::EXCLUSION;

	if ((planes_[FrustumPlane::RIGHT].distance(s.center())) > s.radius())
		return CullType::EXCLUSION;

	if ((planes_[FrustumPlane::LEFT].distance(s.center())) > s.radius())
		return CullType::EXCLUSION;

	if ((planes_[FrustumPlane::TOP].distance(s.center())) > s.radius())
		return CullType::EXCLUSION;

	if ((planes_[FrustumPlane::BOTTOM].distance(s.center())) > s.radius())
		return CullType::EXCLUSION;
	
	return CullType::OVERLAP;
}

X_INLINE CullType::Enum XFrustum::cullSphere_ExactT(const Sphere& s) const
{
	float32_t nc, rc, lc, tc, bc, cc;

	if ((nc = planes_[FrustumPlane::NEAR].distance(s.center())) > s.radius())
		return CullType::EXCLUSION;

	if ((rc = planes_[FrustumPlane::FAR].distance(s.center())) > s.radius())
		return CullType::EXCLUSION;

	if ((lc = planes_[FrustumPlane::RIGHT].distance(s.center())) > s.radius())
		return CullType::EXCLUSION;

	if ((tc = planes_[FrustumPlane::LEFT].distance(s.center())) > s.radius())
		return CullType::EXCLUSION;

	if ((bc = planes_[FrustumPlane::TOP].distance(s.center())) > s.radius())
		return CullType::EXCLUSION;

	if ((cc = planes_[FrustumPlane::BOTTOM].distance(s.center())) > s.radius())
		return CullType::EXCLUSION;

	//now we have to check if it is completely in frustum
	float32_t r = -s.radius();
	if (nc>r) return CullType::OVERLAP;
	if (lc>r) return CullType::OVERLAP;
	if (rc>r) return CullType::OVERLAP;
	if (tc>r) return CullType::OVERLAP;
	if (bc>r) return CullType::OVERLAP;
	if (cc>r) return CullType::OVERLAP;
	return CullType::INCLUSION;
}

// ---------------------------------------------------------------


X_INLINE void XFrustum::GetFrustumVertices(std::array<Vec3f, 8>& verts) const
{
	Matrix33f m33 = Matrix33f(mat_);
	Vec3f pos = getPosition();


	verts[0] = m33*Vec3f(+edge_flt_.x, +edge_flt_.y, +edge_flt_.z) + pos;
	verts[1] = m33*Vec3f(+edge_flt_.x, +edge_flt_.y, -edge_flt_.z) + pos;
	verts[2] = m33*Vec3f(-edge_flt_.x, +edge_flt_.y, -edge_flt_.z) + pos;
	verts[3] = m33*Vec3f(-edge_flt_.x, +edge_flt_.y, +edge_flt_.z) + pos;

	verts[4] = m33*Vec3f(+edge_nlt_.x, +edge_nlt_.y, +edge_nlt_.z) + pos;
	verts[5] = m33*Vec3f(+edge_nlt_.x, +edge_nlt_.y, -edge_nlt_.z) + pos;
	verts[6] = m33*Vec3f(-edge_nlt_.x, +edge_nlt_.y, -edge_nlt_.z) + pos;
	verts[7] = m33*Vec3f(-edge_nlt_.x, +edge_nlt_.y, +edge_nlt_.z) + pos;
}


X_INLINE void XFrustum::GetFrustumVertices(std::array<Vec3f, 12>& verts) const
{
	Matrix33f m33 = Matrix33f(mat_);
	Vec3f pos = getPosition();


	verts[0] = m33*Vec3f(+edge_flt_.x, +edge_flt_.y, +edge_flt_.z) + pos;
	verts[1] = m33*Vec3f(+edge_flt_.x, +edge_flt_.y, -edge_flt_.z) + pos;
	verts[2] = m33*Vec3f(-edge_flt_.x, +edge_flt_.y, -edge_flt_.z) + pos;
	verts[3] = m33*Vec3f(-edge_flt_.x, +edge_flt_.y, +edge_flt_.z) + pos;

	verts[4] = m33*Vec3f(+edge_plt_.x, +edge_plt_.y, +edge_plt_.z) + pos;
	verts[5] = m33*Vec3f(+edge_plt_.x, +edge_plt_.y, -edge_plt_.z) + pos;
	verts[6] = m33*Vec3f(-edge_plt_.x, +edge_plt_.y, -edge_plt_.z) + pos;
	verts[7] = m33*Vec3f(-edge_plt_.x, +edge_plt_.y, +edge_plt_.z) + pos;

	verts[8] = m33*Vec3f(+edge_nlt_.x, +edge_nlt_.y, +edge_nlt_.z) + pos;
	verts[9] = m33*Vec3f(+edge_nlt_.x, +edge_nlt_.y, -edge_nlt_.z) + pos;
	verts[10] = m33*Vec3f(-edge_nlt_.x, +edge_nlt_.y, -edge_nlt_.z) + pos;
	verts[11] = m33*Vec3f(-edge_nlt_.x, +edge_nlt_.y, +edge_nlt_.z) + pos;
}

// ---------------------------------------------------------------
namespace Frustum
{
	extern uint8_t BoxSides[];
}


X_INLINE CullType::Enum XFrustum::AdditionalCheck(const AABB& aabb) const
{
	Vec3<float32_t> m(aabb.center()); // (aabb.min + aabb.max)*0.5;
	uint32 o = 1; //will be reset to 0 if center is outside

	o &= math<float>::isneg(planes_[0].distance(m));
	o &= math<float>::isneg(planes_[2].distance(m));
	o &= math<float>::isneg(planes_[3].distance(m));
	o &= math<float>::isneg(planes_[4].distance(m));
	o &= math<float>::isneg(planes_[5].distance(m));
	o &= math<float>::isneg(planes_[1].distance(m));
	
	//if obb-center is in view-frustum, then stop further calculation
	if (o) 
		return CullType::OVERLAP; 

	Vec3<float32_t> vmin(aabb.min - getPosition());  //AABB in camera-space
	Vec3<float32_t> vmax(aabb.max - getPosition());  //AABB in camera-space

	uint32 frontx8 = 0; // make the flags using the fact that the upper bit in f32 is its sign

	union f64_u
	{
		float64_t floatVal;
		int64 intVal;
	};

	f64_u uminx, uminy, uminz, umaxx, umaxy, umaxz;

	uminx.floatVal = vmin.x;	
	uminy.floatVal = vmin.y;	
	uminz.floatVal = vmin.z;

	umaxx.floatVal = vmax.x;	
	umaxy.floatVal = vmax.y;	
	umaxz.floatVal = vmax.z;

	frontx8 |= (-uminx.intVal >> 0x3f) & 0x008; 
	frontx8 |= (umaxx.intVal >> 0x3f) & 0x010; 
	frontx8 |= (-uminy.intVal >> 0x3f) & 0x020;
	frontx8 |= (umaxy.intVal >> 0x3f) & 0x040; 
	frontx8 |= (-uminz.intVal >> 0x3f) & 0x080; 
	frontx8 |= (umaxz.intVal >> 0x3f) & 0x100;

	//check if camera is inside the aabb
	if (frontx8 == 0)	
		return CullType::OVERLAP; //AABB is patially visible

	Vec3<float32_t> v[8] = {
		Vec3<float32_t>(vmin.x, vmin.y, vmin.z),
		Vec3<float32_t>(vmax.x, vmin.y, vmin.z),
		Vec3<float32_t>(vmin.x, vmax.y, vmin.z),
		Vec3<float32_t>(vmax.x, vmax.y, vmin.z),
		Vec3<float32_t>(vmin.x, vmin.y, vmax.z),
		Vec3<float32_t>(vmax.x, vmin.y, vmax.z),
		Vec3<float32_t>(vmin.x, vmax.y, vmax.z),
		Vec3<float32_t>(vmax.x, vmax.y, vmax.z)
	};

	// find the silhouette-vertices of the AABB            
	uint32 p0 = Frustum::BoxSides[frontx8 + 0];
	uint32 p1 = Frustum::BoxSides[frontx8 + 1];
	uint32 p2 = Frustum::BoxSides[frontx8 + 2];
	uint32 p3 = Frustum::BoxSides[frontx8 + 3];
	uint32 p4 = Frustum::BoxSides[frontx8 + 4];
	uint32 p5 = Frustum::BoxSides[frontx8 + 5];
	uint32 sideamount = Frustum::BoxSides[frontx8 + 7];

	Vec3f cltp = proVerts[PlaneVert::TLEFT];
	Vec3f crtp = proVerts[PlaneVert::TRIGHT];
	Vec3f clbp = proVerts[PlaneVert::BLEFT];
	Vec3f crbp = proVerts[PlaneVert::BRIGHT];

	if (sideamount == 4) 
	{
		// we take the 4 vertices of projection-plane in cam-space,       
		// and clip them against the 4 side-frustum-planes of the AABB        
		Vec3f s0 = v[p0].cross(v[p1]);
		if ((s0 | cltp)>0 && (s0 | crtp)>0 && (s0 | crbp)>0 && (s0 | clbp)>0) 
			return CullType::EXCLUSION;

		Vec3f s1 = v[p1].cross(v[p2]);
		if ((s1 | cltp)>0 && (s1 | crtp)>0 && (s1 | crbp)>0 && (s1 | clbp)>0)
			return CullType::EXCLUSION;

		Vec3f s2 = v[p2].cross(v[p3]);
		if ((s2 | cltp)>0 && (s2 | crtp)>0 && (s2 | crbp)>0 && (s2 | clbp)>0) 
			return CullType::EXCLUSION;

		Vec3f s3 = v[p3].cross(v[p0]);
		if ((s3 | cltp)>0 && (s3 | crtp)>0 && (s3 | crbp)>0 && (s3 | clbp)>0) 
			return CullType::EXCLUSION;
	}

	if (sideamount == 6) 
	{
		// we take the 4 vertices of projection-plane in cam-space,       
		// and clip them against the 6 side-frustum-planes of the AABB     
		Vec3f s0 = v[p0].cross(v[p1]);
		if ((s0 | cltp)>0 && (s0 | crtp)>0 && (s0 | crbp)>0 && (s0 | clbp)>0) 
			return CullType::EXCLUSION;

		Vec3f s1 = v[p1].cross(v[p2]);
		if ((s1 | cltp)>0 && (s1 | crtp)>0 && (s1 | crbp)>0 && (s1 | clbp)>0) 
			return CullType::EXCLUSION;

		Vec3f s2 = v[p2].cross(v[p3]);
		if ((s2 | cltp)>0 && (s2 | crtp)>0 && (s2 | crbp)>0 && (s2 | clbp)>0) 
			return CullType::EXCLUSION;

		Vec3f s3 = v[p3].cross(v[p4]);
		if ((s3 | cltp)>0 && (s3 | crtp)>0 && (s3 | crbp)>0 && (s3 | clbp)>0) 
			return CullType::EXCLUSION;

		Vec3f s4 = v[p4].cross(v[p5]);
		if ((s4 | cltp)>0 && (s4 | crtp)>0 && (s4 | crbp)>0 && (s4 | clbp)>0) 
			return CullType::EXCLUSION;

		Vec3f s5 = v[p5].cross(v[p0]);
		if ((s5 | cltp)>0 && (s5 | crtp)>0 && (s5 | crbp)>0 && (s5 | clbp)>0) 
			return CullType::EXCLUSION;
	}


	return CullType::OVERLAP; //AABB is patially visible
}


X_INLINE CullType::Enum XFrustum::AdditionalCheck(const OBB& obb, float32_t scale) const
{
	Vec3f CamInOBBSpace = getPosition();
//	Vec3f iCamPos = (-CamInOBBSpace)*obb.orientation();
	Vec3f iCamPos = obb.orientation() * (-CamInOBBSpace);
	uint32 front8 = 0;
	AABB aabb = AABB((obb.center() - obb.halfVec())*scale, (obb.center() + obb.halfVec())*scale);

	if (iCamPos.x<aabb.min.x)  front8 |= 0x008;
	if (iCamPos.x>aabb.max.x)  front8 |= 0x010;
	if (iCamPos.y<aabb.min.y)  front8 |= 0x020;
	if (iCamPos.y>aabb.max.y)  front8 |= 0x040;
	if (iCamPos.z<aabb.min.z)  front8 |= 0x080;
	if (iCamPos.z>aabb.max.z)  front8 |= 0x100;

	if (front8 == 0) 
		return CullType::OVERLAP;

	// the transformed OBB-vertices in cam-space
	Vec3f v[8] = {
		obb.orientation()*Vec3f(aabb.min.x, aabb.min.y, aabb.min.z) + CamInOBBSpace,
		obb.orientation()*Vec3f(aabb.max.x, aabb.min.y, aabb.min.z) + CamInOBBSpace,
		obb.orientation()*Vec3f(aabb.min.x, aabb.max.y, aabb.min.z) + CamInOBBSpace,
		obb.orientation()*Vec3f(aabb.max.x, aabb.max.y, aabb.min.z) + CamInOBBSpace,
		obb.orientation()*Vec3f(aabb.min.x, aabb.min.y, aabb.max.z) + CamInOBBSpace,
		obb.orientation()*Vec3f(aabb.max.x, aabb.min.y, aabb.max.z) + CamInOBBSpace,
		obb.orientation()*Vec3f(aabb.min.x, aabb.max.y, aabb.max.z) + CamInOBBSpace,
		obb.orientation()*Vec3f(aabb.max.x, aabb.max.y, aabb.max.z) + CamInOBBSpace
	};

	// find the silhouette-vertices of the OBB            
	uint32 p0 = Frustum::BoxSides[front8 + 0];
	uint32 p1 = Frustum::BoxSides[front8 + 1];
	uint32 p2 = Frustum::BoxSides[front8 + 2];
	uint32 p3 = Frustum::BoxSides[front8 + 3];
	uint32 p4 = Frustum::BoxSides[front8 + 4];
	uint32 p5 = Frustum::BoxSides[front8 + 5];
	uint32 sideamount = Frustum::BoxSides[front8 + 7];


	Vec3f cltp = proVerts[PlaneVert::TLEFT];
	Vec3f crtp = proVerts[PlaneVert::TRIGHT];
	Vec3f clbp = proVerts[PlaneVert::BLEFT];
	Vec3f crbp = proVerts[PlaneVert::BRIGHT];

	if (sideamount == 4) 
	{
		// we take the 4 vertices of projection-plane in cam-space,       
		// and clip them against the 4 side-frustum-planes of the OBB       
		Vec3f s0 = v[p0].cross(v[p1]);
		if (((s0 | cltp) >= 0) && ((s0 | crtp) >= 0) && ((s0 | crbp) >= 0) && ((s0 | clbp) >= 0)) 
			return CullType::EXCLUSION;

		Vec3f s1 = v[p1].cross(v[p2]);
		if (((s1 | cltp) >= 0) && ((s1 | crtp) >= 0) && ((s1 | crbp) >= 0) && ((s1 | clbp) >= 0)) 
			return CullType::EXCLUSION;

		Vec3f s2 = v[p2].cross(v[p3]);
		if (((s2 | cltp) >= 0) && ((s2 | crtp) >= 0) && ((s2 | crbp) >= 0) && ((s2 | clbp) >= 0)) 
			return CullType::EXCLUSION;

		Vec3f s3 = v[p3].cross(v[p0]);
		if (((s3 | cltp) >= 0) && ((s3 | crtp) >= 0) && ((s3 | crbp) >= 0) && ((s3 | clbp) >= 0)) 
			return CullType::EXCLUSION;
	}

	if (sideamount == 6) 
	{
		// we take the 4 vertices of projection-plane in cam-space,       
		// and clip them against the 6 side-frustum-planes of the OBB      
		Vec3f s0 = v[p0].cross(v[p1]);
		if (((s0 | cltp) >= 0) && ((s0 | crtp) >= 0) && ((s0 | crbp) >= 0) && ((s0 | clbp) >= 0)) 
			return CullType::EXCLUSION;

		Vec3f s1 = v[p1].cross(v[p2]);
		if (((s1 | cltp) >= 0) && ((s1 | crtp) >= 0) && ((s1 | crbp) >= 0) && ((s1 | clbp) >= 0)) 
			return CullType::EXCLUSION;

		Vec3f s2 = v[p2].cross(v[p3]);
		if (((s2 | cltp) >= 0) && ((s2 | crtp) >= 0) && ((s2 | crbp) >= 0) && ((s2 | clbp) >= 0)) 
			return CullType::EXCLUSION;

		Vec3f s3 = v[p3].cross(v[p4]);
		if (((s3 | cltp) >= 0) && ((s3 | crtp) >= 0) && ((s3 | crbp) >= 0) && ((s3 | clbp) >= 0)) 
			return CullType::EXCLUSION;

		Vec3f s4 = v[p4].cross(v[p5]);
		if (((s4 | cltp) >= 0) && ((s4 | crtp) >= 0) && ((s4 | crbp) >= 0) && ((s4 | clbp) >= 0)) 
			return CullType::EXCLUSION;

		Vec3f s5 = v[p5].cross(v[p0]);
		if (((s5 | cltp) >= 0) && ((s5 | crtp) >= 0) && ((s5 | crbp) >= 0) && ((s5 | clbp) >= 0)) 
			return CullType::EXCLUSION;
	}

	//now we are 100% sure that the OBB is visible on the screen
	return CullType::OVERLAP;
}



X_INLINE float32_t XFrustum::getFov(void) const
{ 
	return fov_;
}

X_INLINE float32_t XFrustum::getProjectionRatio(void) const
{
	return projectionRatio_; 
}

X_INLINE void XFrustum::setAngles(const Vec3f& angles)
{
	setAxis(Matrix33f::createRotation(angles));
}

X_INLINE Planef XFrustum::getFrustumPlane(FrustumPlane::Enum pl)
{
	return planes_[pl];
}

X_INLINE const Planef& XFrustum::getFrustumPlane(FrustumPlane::Enum pl) const
{
	return planes_[pl];
}
