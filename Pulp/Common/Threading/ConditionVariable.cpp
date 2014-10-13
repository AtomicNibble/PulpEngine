#include <EngineCommon.h>

#include "ConditionVariable.h"

X_NAMESPACE_BEGIN(core)




ConditionVariable::ConditionVariable(void)
{
	InitializeConditionVariable(&m_condVar);
}

void ConditionVariable::NotifyOne(void)
{
	WakeConditionVariable(&m_condVar);
}

void ConditionVariable::NotifyAll(void)
{
	WakeAllConditionVariable(&m_condVar);
}

void ConditionVariable::Wait(CriticalSection& criticalSection)
{
//	CriticalSection::ScopedLock lock(criticalSection);
	SleepConditionVariableCS(&m_condVar, criticalSection.GetNativeObject(), INFINITE);
}




X_NAMESPACE_END