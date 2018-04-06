#pragma once

#ifndef X_JOBSYSTEM_I_H_
#define X_JOBSYSTEM_I_H_

#include "Time\TimeVal.h"

X_NAMESPACE_BEGIN(core)

#define X_DECLARE_JOB_ENTRY(name) void name(void* pParam, uint32_t workerIdx)

typedef void (*Job)(void* pParam, uint32_t workerIdx);

struct JobDecl
{
    JobDecl()
    {
        pJobFunc = nullptr;
        pParam = nullptr;
    }
    JobDecl(Job pJob, void* pParam)
    {
        this->pJobFunc = pJob;
        this->pParam = pParam;
    }

    Job pJobFunc;
    void* pParam;
    TimeVal execTime;
};

X_DECLARE_ENUM(JobPriority)
(
    HIGH,
    NORMAL,
    NONE);

struct IJobSystem
{
public:
    virtual ~IJobSystem(){};

    virtual bool StartUp(void) X_ABSTRACT;
    virtual void ShutDown(void) X_ABSTRACT;

    virtual void AddJob(const JobDecl job,
        JobPriority::Enum priority = JobPriority::NORMAL) X_ABSTRACT;
    virtual void AddJobs(JobDecl* pJobs, size_t numJobs,
        JobPriority::Enum priority = JobPriority::NORMAL) X_ABSTRACT;

    virtual void waitForAllJobs(void) X_ABSTRACT;

    virtual int32_t numThreads(void) const X_ABSTRACT;
};

X_NAMESPACE_END

#endif // !X_JOBSYSTEM_I_H_
