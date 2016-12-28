#pragma once


X_NAMESPACE_BEGIN(physics)


X_INLINE Vec3f Vec3FromPhysx(const physx::PxVec3& vec)
{
	return Vec3f(vec.x, vec.y, vec.z);
}

X_INLINE Vec3f Vec3FromPx3(const physx::PxVec3& vec)
{
	return Vec3f(vec.x, vec.y, vec.z);
}

X_INLINE physx::PxVec3 Px3FromVec3(const Vec3f& vec)
{
	return physx::PxVec3(vec.x, vec.y, vec.z);
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
