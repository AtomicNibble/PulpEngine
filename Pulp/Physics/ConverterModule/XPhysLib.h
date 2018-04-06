#pragma once

#include <IPhysics.h>

#include "Cooking.h"
#include "Util/Logger.h"

X_NAMESPACE_BEGIN(physics)

class XPhysLib : public IPhysLib
{
public:
    XPhysLib(core::MemoryArenaBase* arena);
    ~XPhysLib() X_FINAL;

    bool init(void);

    // IConverter
    const char* getOutExtension(void) const X_FINAL;
    bool Convert(IConverterHost& host, int32_t assetId, ConvertArgs& args, const OutPath& destPath) X_FINAL;
    // ~IConverter

    // IPhysLib
    IPhysicsCooking* getCooking(void) X_FINAL;
    // ~IPhysLib

private:
    physx::PxFoundation* pFoundation_;
    PhysxLogger logger_;
    PhysCooking cooking_;
};

X_NAMESPACE_END