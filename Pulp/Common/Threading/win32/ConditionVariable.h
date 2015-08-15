#pragma once

#ifndef _X_THREADING_SYNC_CONDIRIONAL_H_
#define _X_THREADING_SYNC_CONDIRIONAL_H_

X_NAMESPACE_BEGIN(core)

class CriticalSection;

class ConditionVariable
{
public:
	ConditionVariable(void);

	void NotifyOne(void);
	void NotifyAll(void);

	void Wait(CriticalSection& criticalSection);

private:
	CONDITION_VARIABLE m_condVar;
};


X_NAMESPACE_END

#endif // !_X_THREADING_SYNC_CONDIRIONAL_H_
