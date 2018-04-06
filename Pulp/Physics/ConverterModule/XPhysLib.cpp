#include "stdafx.h"
#include "XPhysLib.h"
#include "Util/DelayLoadHook.h"

X_NAMESPACE_BEGIN(physics)

namespace
{
    physx::PxDefaultAllocator gDefaultAllocatorCallback;

} // namespace

XPhysLib::XPhysLib(core::MemoryArenaBase* arena) :
    pFoundation_(nullptr),
    cooking_(arena)
{
}

XPhysLib::~XPhysLib()
{
    cooking_.shutDown();

    core::SafeRelease(pFoundation_);
}

bool XPhysLib::init(void)
{
    X_ASSERT(pFoundation_ == nullptr, "Init called twice")(pFoundation_);

    physx::PxSetPhysXDelayLoadHook(&gDelayLoadHook);
    physx::PxSetPhysXCookingDelayLoadHook(&gDelayLoadHook);

    pFoundation_ = PxCreateFoundation(PX_PHYSICS_VERSION,
        gDefaultAllocatorCallback,
        logger_);

    if (!pFoundation_) {
        X_ERROR("Physics", "Failed to create foundation");
        return false;
    }

    physx::PxTolerancesScale scale;
    scale.length = physics::SCALE_LENGTH;
    scale.mass = physics::SCALE_MASS;
    scale.speed = physics::SCALE_SPEED;

    if (!cooking_.init(scale, *pFoundation_, CookingMode::VerySlow)) {
        X_ERROR("Physics", "Failed to init cooking");
        return false;
    }

    return true;
}

const char* XPhysLib::getOutExtension(void) const
{
    return "phys";
}

bool XPhysLib::Convert(IConverterHost& host, int32_t assetId, ConvertArgs& args, const OutPath& destPath)
{
    X_UNUSED(host);
    X_UNUSED(assetId);
    X_UNUSED(args);
    X_UNUSED(destPath);
    return false;
}

IPhysicsCooking* XPhysLib::getCooking(void)
{
    return &cooking_;
}

X_NAMESPACE_END