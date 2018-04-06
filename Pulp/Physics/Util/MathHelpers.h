#pragma once

#include <IPhysics.h>

X_NAMESPACE_BEGIN(physics)

// from physx

X_INLINE const Vec3f& Vec3FromPx3(const physx::PxVec3& vec)
{
    return reinterpret_cast<const Vec3f&>(vec);
}
X_INLINE Vec3f& Vec3FromPx3(physx::PxVec3& vec)
{
    return reinterpret_cast<Vec3f&>(vec);
}

X_INLINE const Vec3d& Vec3FromPx3Ext(const physx::PxExtendedVec3& vec)
{
    return reinterpret_cast<const Vec3d&>(vec);
}
X_INLINE Vec3d& Vec3FromPx3Ext(physx::PxExtendedVec3& vec)
{
    return reinterpret_cast<Vec3d&>(vec);
}

X_INLINE const Quatf& QuatFromPxQuat(const physx::PxQuat& quat)
{
    return reinterpret_cast<const Quatf&>(quat);
}
X_INLINE Quatf& QuatFromPxQuat(physx::PxQuat& quat)
{
    return reinterpret_cast<Quatf&>(quat);
}

X_INLINE const Transformf& QuatTransFromPxTrans(const physx::PxTransform& trans)
{
    return reinterpret_cast<const Transformf&>(trans);
}
X_INLINE Transformf& QuatTransFromPxTrans(physx::PxTransform& trans)
{
    return reinterpret_cast<Transformf&>(trans);
}

// to physx
X_INLINE const physx::PxVec3& Px3FromVec3(const Vec3f& vec)
{
    return reinterpret_cast<const physx::PxVec3&>(vec);
}
X_INLINE physx::PxVec3& Px3FromVec3(Vec3f& vec)
{
    return reinterpret_cast<physx::PxVec3&>(vec);
}

X_INLINE const physx::PxExtendedVec3& Px3ExtFromVec3(const Vec3d& vec)
{
    return reinterpret_cast<const physx::PxExtendedVec3&>(vec);
}
X_INLINE physx::PxExtendedVec3& Px3ExtFromVec3(Vec3d& vec)
{
    return reinterpret_cast<physx::PxExtendedVec3&>(vec);
}

X_INLINE const physx::PxQuat& PxQuatFromQuat(const Quatf& quat)
{
    return reinterpret_cast<const physx::PxQuat&>(quat);
}
X_INLINE physx::PxQuat& PxQuatFromQuat(Quatf& quat)
{
    return reinterpret_cast<physx::PxQuat&>(quat);
}

X_INLINE const physx::PxTransform& PxTransFromQuatTrans(const Transformf& myTrans)
{
    return reinterpret_cast<const physx::PxTransform&>(myTrans);
}
X_INLINE physx::PxTransform& PxTransFromQuatTrans(Transformf& myTrans)
{
    return reinterpret_cast<physx::PxTransform&>(myTrans);
}

X_INLINE const physx::PxBounds3& PxBounds3FromAABB(const AABB& aabb)
{
    return reinterpret_cast<const physx::PxBounds3&>(aabb);
}
X_INLINE physx::PxBounds3& PxBounds3FromAABB(AABB& aabb)
{
    return reinterpret_cast<physx::PxBounds3&>(aabb);
}

// Geo
X_INLINE const physx::PxGeometry& PxGeoFromGeo(const GeometryBase& geo)
{
    return reinterpret_cast<const physx::PxBoxGeometry&>(geo);
}
X_INLINE physx::PxGeometry& PxGeoFromGeo(GeometryBase& geo)
{
    return reinterpret_cast<physx::PxGeometry&>(geo);
}

X_INLINE const physx::PxBoxGeometry& PxBoxGeoFromBoxGeo(const BoxGeometry& box)
{
    return reinterpret_cast<const physx::PxBoxGeometry&>(box);
}
X_INLINE physx::PxBoxGeometry& PxBoxGeoFromBoxGeo(BoxGeometry& box)
{
    return reinterpret_cast<physx::PxBoxGeometry&>(box);
}

X_INLINE const physx::PxSphereGeometry& PxSphereGeoFromSphereGeo(const SphereGeometry& sphere)
{
    return reinterpret_cast<const physx::PxSphereGeometry&>(sphere);
}
X_INLINE physx::PxSphereGeometry& PxSphereGeoFromSphereGeo(SphereGeometry& sphere)
{
    return reinterpret_cast<physx::PxSphereGeometry&>(sphere);
}

X_INLINE const physx::PxPlaneGeometry& PxPlaneGeoFromPlaneGeo(const PlaneGeometry& plane)
{
    return reinterpret_cast<const physx::PxPlaneGeometry&>(plane);
}
X_INLINE physx::PxPlaneGeometry& PxPlaneGeoFromPlaneGeo(PlaneGeometry& plane)
{
    return reinterpret_cast<physx::PxPlaneGeometry&>(plane);
}

X_INLINE const physx::PxCapsuleGeometry& PxCapsuleGeoFromCapsuleGeo(const CapsuleGeometry& cap)
{
    return reinterpret_cast<const physx::PxCapsuleGeometry&>(cap);
}
X_INLINE physx::PxCapsuleGeometry& PxCapsuleGeoFromCapsuleGeo(CapsuleGeometry& cap)
{
    return reinterpret_cast<physx::PxCapsuleGeometry&>(cap);
}

X_NAMESPACE_END
