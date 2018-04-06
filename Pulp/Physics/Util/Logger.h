#pragma once

X_NAMESPACE_BEGIN(physics)

class PhysxLogger : public physx::PxErrorCallback
{
public:
    PhysxLogger() = default;
    ~PhysxLogger() X_OVERRIDE = default;

    void reportError(physx::PxErrorCode::Enum code, const char* message, const char* file, int line) X_FINAL;
};

X_NAMESPACE_END