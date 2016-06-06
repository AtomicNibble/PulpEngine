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


X_NAMESPACE_END
