#pragma once

#include <IPhysics.h>

X_NAMESPACE_BEGIN(physics)

namespace filter
{
    physx::PxFilterFlags FilterShader(
        physx::PxFilterObjectAttributes attributes0,
        physx::PxFilterData filterData0,
        physx::PxFilterObjectAttributes attributes1,
        physx::PxFilterData filterData1,
        physx::PxPairFlags& pairFlags,
        const void* constantBlock,
        physx::PxU32 constantBlockSize);

    bool GetGroupCollisionFlag(const GroupFlag::Enum group1, const GroupFlag::Enum group2);
    void SetGroupCollisionFlag(const GroupFlag::Enum group1, const GroupFlag::Enum group2, const bool enable);

    void SetGroup(physx::PxActor& actor, const GroupFlag::Enum group);
    void SetGroupMask(physx::PxActor& actor, const GroupFlags groups);

} // namespace filter

X_NAMESPACE_END