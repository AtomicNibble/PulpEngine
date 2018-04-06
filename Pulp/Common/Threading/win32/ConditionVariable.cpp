#include <EngineCommon.h>

#include "ConditionVariable.h"
#include "SharedLock.h"

X_NAMESPACE_BEGIN(core)

ConditionVariable::ConditionVariable(void)
{
    InitializeConditionVariable(&condVar_);
}

void ConditionVariable::NotifyOne(void)
{
    WakeConditionVariable(&condVar_);
}

void ConditionVariable::NotifyAll(void)
{
    WakeAllConditionVariable(&condVar_);
}

void ConditionVariable::Wait(CriticalSection& criticalSection)
{
    SleepConditionVariableCS(&condVar_, criticalSection.GetNativeObject(), INFINITE);
}

void ConditionVariable::Wait(SharedLock& sharedLock, bool isExclusive)
{
    ULONG flags = CONDITION_VARIABLE_LOCKMODE_SHARED;

    if (isExclusive) {
        flags = 0;
    }

    SleepConditionVariableSRW(&condVar_, sharedLock.GetNativeObject(), INFINITE, flags);
}

X_NAMESPACE_END