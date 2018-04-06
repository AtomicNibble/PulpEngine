#pragma once

#include <foundation\PxAssert.h>

X_NAMESPACE_BEGIN(physics)

class AssetHandler : public physx::PxAssertHandler
{
public:
    AssetHandler() = default;
    ~AssetHandler() X_OVERRIDE = default;

    void operator()(const char* exp, const char* file, int line, bool& ignore) X_FINAL;
};

extern AssetHandler gAssetHandler;

X_NAMESPACE_END