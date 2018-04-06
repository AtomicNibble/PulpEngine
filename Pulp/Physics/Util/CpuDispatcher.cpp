#include "stdafx.h"
#include "CpuDispatcher.h"

#include <Threading\JobSystem2.h>

X_NAMESPACE_BEGIN(physics)

PhysxCpuDispacher::PhysxCpuDispacher(core::V2::JobSystem& jobSystem) :
    jobSystem_(jobSystem)
{
}

void PhysxCpuDispacher::submitTask(physx::PxBaseTask& task)
{
    core::V2::Job* pJob = jobSystem_.CreateJob(RunTask, &task JOB_SYS_SUB_ARG(core::profiler::SubSys::PHYSICS));

    jobSystem_.Run(pJob);
}

physx::PxU32 PhysxCpuDispacher::getWorkerCount() const
{
    return jobSystem_.GetThreadCount();
}

void PhysxCpuDispacher::RunTask(core::V2::JobSystem&, size_t, core::V2::Job*, void* pData)
{
    physx::PxBaseTask* pTask = reinterpret_cast<physx::PxBaseTask*>(pData);

    pTask->run();
    pTask->release();
}

X_NAMESPACE_END