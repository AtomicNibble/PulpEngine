#include "stdafx.h"
#include "MathHelpers.h"

X_NAMESPACE_BEGIN(physics)

namespace
{
    X_ENSURE_SIZE(Vec3f, sizeof(physx::PxVec3));
    X_ENSURE_SIZE(Vec3d, sizeof(physx::PxExtendedVec3));
    X_ENSURE_SIZE(Quatf, sizeof(physx::PxQuat));
    X_ENSURE_SIZE(AABB, sizeof(physx::PxBounds3));
    X_ENSURE_SIZE(Transformf, sizeof(physx::PxTransform));

    X_ENSURE_SIZE(GeometryBase, sizeof(physx::PxGeometry));
    X_ENSURE_SIZE(BoxGeometry, sizeof(physx::PxBoxGeometry));
    X_ENSURE_SIZE(SphereGeometry, sizeof(physx::PxSphereGeometry));
    X_ENSURE_SIZE(PlaneGeometry, sizeof(physx::PxPlaneGeometry));
    X_ENSURE_SIZE(CapsuleGeometry, sizeof(physx::PxCapsuleGeometry));

    static_assert(X_OFFSETOF(Vec3f, x) == X_OFFSETOF(physx::PxVec3, x), "Offset don't match");
    static_assert(X_OFFSETOF(Vec3f, y) == X_OFFSETOF(physx::PxVec3, y), "Offset don't match");
    static_assert(X_OFFSETOF(Vec3f, z) == X_OFFSETOF(physx::PxVec3, z), "Offset don't match");

    static_assert(X_OFFSETOF(Vec3d, x) == X_OFFSETOF(physx::PxExtendedVec3, x), "Offset don't match");
    static_assert(X_OFFSETOF(Vec3d, y) == X_OFFSETOF(physx::PxExtendedVec3, y), "Offset don't match");
    static_assert(X_OFFSETOF(Vec3d, z) == X_OFFSETOF(physx::PxExtendedVec3, z), "Offset don't match");

    static_assert(X_OFFSETOF(Quatf, v.x) == X_OFFSETOF(physx::PxQuat, x), "Offset don't match");
    static_assert(X_OFFSETOF(Quatf, v.y) == X_OFFSETOF(physx::PxQuat, y), "Offset don't match");
    static_assert(X_OFFSETOF(Quatf, v.z) == X_OFFSETOF(physx::PxQuat, z), "Offset don't match");
    static_assert(X_OFFSETOF(Quatf, w) == X_OFFSETOF(physx::PxQuat, w), "Offset don't match");

    static_assert(X_OFFSETOF(AABB, min) == X_OFFSETOF(physx::PxBounds3, minimum), "Offset don't match");
    static_assert(X_OFFSETOF(AABB, max) == X_OFFSETOF(physx::PxBounds3, maximum), "Offset don't match");

    static_assert(X_OFFSETOF(Transformf, quat) == X_OFFSETOF(physx::PxTransform, q), "Offset don't match");
    static_assert(X_OFFSETOF(Transformf, pos) == X_OFFSETOF(physx::PxTransform, p), "Offset don't match");

} // namespace

X_NAMESPACE_END
