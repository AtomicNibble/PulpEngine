#include "EngineCommon.h"

#include "CriticalSection.h"

X_NAMESPACE_BEGIN(core)


CriticalSection::CriticalSection(void)
{
	InitializeCriticalSection( &m_cs );
}

/// \brief Initializes the critical section with a certain spin count.
/// \remark Entering the critical section will first try spinning the given number of times before finally acquiring
/// the critical section if spinning was unsuccessful.
CriticalSection::CriticalSection(unsigned int spinCount)
{
	InitializeCriticalSectionAndSpinCount( &m_cs, spinCount );
}

/// Releases OS resources of the critical section.
CriticalSection::~CriticalSection(void)
{
	DeleteCriticalSection( &m_cs );
}

/// Enters the critical section.
void CriticalSection::Enter(void)
{
	EnterCriticalSection( &m_cs );
}

/// Tries to enter the critical section, and returns whether the operation was successful.
bool CriticalSection::TryEnter(void)
{
	return TryEnterCriticalSection( &m_cs ) != 0;
}

/// Leaves the critical section.
void CriticalSection::Leave(void)
{
	LeaveCriticalSection( &m_cs );
}





X_NAMESPACE_END