#pragma once


X_NAMESPACE_BEGIN(physics)


// from physx

X_INLINE Vec3f Vec3FromPhysx(const physx::PxVec3& vec)
{
	return Vec3f(vec.x, vec.y, vec.z);
}

X_INLINE Vec3f Vec3FromPx3(const physx::PxVec3& vec)
{
	return Vec3f(vec.x, vec.y, vec.z);
}


X_INLINE Vec3d Vec3FromPx3Ext(const physx::PxExtendedVec3& vec)
{
	return Vec3d(vec.x, vec.y, vec.z);
}


X_INLINE Quatf QuatFromPxQuat(const physx::PxQuat& quat)
{
	return Quatf(quat.x, quat.y, quat.z, quat.w);
}

X_INLINE QuatTransf QuatTransFromPxTrans(const physx::PxTransform& trans)
{
	return QuatTransf(Vec3FromPx3(trans.p), QuatFromPxQuat(trans.q));
}

// to physx
X_INLINE physx::PxVec3 Px3FromVec3(const Vec3f& vec)
{
	return physx::PxVec3(vec.x, vec.y, vec.z);
}

X_INLINE physx::PxExtendedVec3 Px3ExtFromVec3(const Vec3d& vec)
{
	return physx::PxExtendedVec3(vec.x, vec.y, vec.z);
}


X_INLINE physx::PxQuat PxQuatFromQuat(const Quatf& quat)
{
	return physx::PxQuat(quat.v.x, quat.v.y, quat.v.z, quat.w);
}

X_INLINE physx::PxTransform PxTransFromQuatTrans(const QuatTransf& myTrans)
{
	physx::PxTransform trans;
	trans.p = Px3FromVec3(myTrans.trans);
	trans.q = PxQuatFromQuat(myTrans.quat);
	return trans;
}

X_INLINE physx::PxBounds3 PxBounds3FromAABB(const AABB& aabb)
{
	physx::PxBounds3 bounds;
	bounds.minimum = Px3FromVec3(aabb.min);
	bounds.maximum = Px3FromVec3(aabb.max);
	return bounds;
}

X_NAMESPACE_END
