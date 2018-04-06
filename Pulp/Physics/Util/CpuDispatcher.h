#pragma once

#include <pxtask\PxCpuDispatcher.h>

X_NAMESPACE_DECLARE(core,
    namespace V2 {
        struct Job;
    });

X_NAMESPACE_BEGIN(physics)

class PhysxCpuDispacher : public physx::PxCpuDispatcher
{
public:
    PhysxCpuDispacher(core::V2::JobSystem& jobSystem);
    ~PhysxCpuDispacher() X_FINAL = default;

private:
    void submitTask(physx::PxBaseTask& task) X_FINAL;
    physx::PxU32 getWorkerCount() const X_FINAL;

    static void RunTask(core::V2::JobSystem&, size_t, core::V2::Job*, void*);

private:
    core::V2::JobSystem& jobSystem_;
};

X_NAMESPACE_END