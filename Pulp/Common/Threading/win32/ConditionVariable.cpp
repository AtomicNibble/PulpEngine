#include <EngineCommon.h>

#include "ConditionVariable.h"

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




X_NAMESPACE_END