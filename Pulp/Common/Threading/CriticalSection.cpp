#include "EngineCommon.h"

#include "CriticalSection.h"

X_NAMESPACE_BEGIN(core)


CriticalSection::CriticalSection(void)
{
	InitializeCriticalSection( &cs_ );
}

/// \brief Initializes the critical section with a certain spin count.
/// \remark Entering the critical section will first try spinning the given number of times before finally acquiring
/// the critical section if spinning was unsuccessful.
CriticalSection::CriticalSection(unsigned int spinCount)
{
	InitializeCriticalSectionAndSpinCount(&cs_, spinCount);
}

/// Releases OS resources of the critical section.
CriticalSection::~CriticalSection(void)
{
	DeleteCriticalSection(&cs_);
}

/// Enters the critical section.
void CriticalSection::Enter(void)
{
	EnterCriticalSection(&cs_);
}

/// Tries to enter the critical section, and returns whether the operation was successful.
bool CriticalSection::TryEnter(void)
{
	return TryEnterCriticalSection(&cs_) != 0;
}

/// Leaves the critical section.
void CriticalSection::Leave(void)
{
	LeaveCriticalSection(&cs_);
}





X_NAMESPACE_END