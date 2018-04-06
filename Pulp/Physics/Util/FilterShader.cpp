#include "stdafx.h"
#include "FilterShader.h"

X_NAMESPACE_BEGIN(physics)

namespace filter
{
    namespace
    {
        struct CollisionBitMap
        {
            X_INLINE CollisionBitMap() :
                enable(true)
            {
            }

            bool operator()() const
            {
                return enable;
            }

            bool& operator=(const bool& v)
            {
                enable = v;
                return enable;
            }

        private:
            bool enable;
        };

        CollisionBitMap gCollisionTable[GroupFlag::FLAGS_COUNT][GroupFlag::FLAGS_COUNT];

        static physx::PxFilterData convert(const GroupFlags& mask)
        {
            physx::PxFilterData fd;
            fd.word1 = physx::PxU32(mask.ToInt());
            return fd;
        }

        static GroupFlags convert(const physx::PxFilterData& fd)
        {
            return GroupFlags(fd.word1);
        }

        X_INLINE uint32_t convertFlagToIdx(const GroupFlag::Enum group)
        {
            uint32_t idx = core::bitUtil::ScanBits(group);
            X_ASSERT(idx != core::bitUtil::NO_BIT_SET, "Flag is invalid")
            (group, idx);
            return idx;
        }

        static void SetGroupCollisionFlag(const uint32_t group1, const uint32_t group2, const bool enable)
        {
            X_ASSERT(group1 < GroupFlag::FLAGS_COUNT, "Index out of range")
            (group1);
            X_ASSERT(group2 < GroupFlag::FLAGS_COUNT, "Index out of range")
            (group2);
            gCollisionTable[group1][group2] = enable;
            gCollisionTable[group2][group1] = enable;
        }

        static bool GetGroupCollisionFlag(const uint32_t group1, const uint32_t group2)
        {
            X_ASSERT(group1 < GroupFlag::FLAGS_COUNT, "Index out of range")
            (group1);
            X_ASSERT(group2 < GroupFlag::FLAGS_COUNT, "Index out of range")
            (group2);
            return gCollisionTable[group1][group2]();
        }

        X_INLINE static void adjustFilterData(bool groupsMask, const physx::PxFilterData& src, physx::PxFilterData& dst)
        {
            if (groupsMask) {
                dst.word1 = src.word1;
            }
            else {
                dst.word0 = src.word0;
            }
        }

        template<bool TGroupsMask>
        static void setFilterData(physx::PxActor& actor, const physx::PxFilterData& fd)
        {
            physx::PxActorType::Enum aType = actor.getType();
            switch (aType) {
                case physx::PxActorType::eRIGID_DYNAMIC:
                case physx::PxActorType::eRIGID_STATIC:
                case physx::PxActorType::eARTICULATION_LINK: {
                    const physx::PxRigidActor& rActor = static_cast<const physx::PxRigidActor&>(actor);

                    physx::PxShape* pShape;
                    for (physx::PxU32 i = 0; i < rActor.getNbShapes(); i++) {
                        rActor.getShapes(&pShape, 1, i);

                        // retrieve current group mask
                        physx::PxFilterData resultFd = pShape->getSimulationFilterData();

                        adjustFilterData(TGroupsMask, fd, resultFd);

                        // set new filter data
                        pShape->setSimulationFilterData(resultFd);
                    }
                } break;

                case physx::PxActorType::eACTOR_COUNT:
                case physx::PxActorType::eACTOR_FORCE_DWORD:
                default:
                    break;
            }
        }

    } // namespace

    physx::PxFilterFlags FilterShader(
        physx::PxFilterObjectAttributes attributes0,
        physx::PxFilterData filterData0,
        physx::PxFilterObjectAttributes attributes1,
        physx::PxFilterData filterData1,
        physx::PxPairFlags& pairFlags,
        const void* constantBlock,
        physx::PxU32 constantBlockSize)
    {
        X_UNUSED(constantBlock);
        X_UNUSED(constantBlockSize);

        if (physx::PxFilterObjectIsTrigger(attributes0) || physx::PxFilterObjectIsTrigger(attributes1)) {
            pairFlags = physx::PxPairFlag::eTRIGGER_DEFAULT;
            return physx::PxFilterFlags();
        }

        // Collision Group
        if (!gCollisionTable[filterData0.word0][filterData1.word0]()) {
            return physx::PxFilterFlag::eSUPPRESS;
        }

        pairFlags = physx::PxPairFlag::eCONTACT_DEFAULT;

        return physx::PxFilterFlags();
    }

    bool GetGroupCollisionFlag(const GroupFlag::Enum group1, const GroupFlag::Enum group2)
    {
        return gCollisionTable[convertFlagToIdx(group1)][convertFlagToIdx(group2)]();
    }

    void SetGroupCollisionFlag(const GroupFlag::Enum group1, const GroupFlag::Enum group2, const bool enable)
    {
        SetGroupCollisionFlag(convertFlagToIdx(group1), convertFlagToIdx(group2), enable);
    }

    void SetGroup(physx::PxActor& actor, const GroupFlag::Enum group)
    {
        physx::PxFilterData fd(convertFlagToIdx(group), 0, 0, 0);
        setFilterData<false>(actor, fd);
    }

    void SetGroupMask(physx::PxActor& actor, const GroupFlags groups)
    {
        physx::PxFilterData fd = convert(groups);
        setFilterData<true>(actor, fd);
    }

} // namespace filter

X_NAMESPACE_END