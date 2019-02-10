#include "EngineCommon.h"

#include "CriticalSection.h"

X_NAMESPACE_BEGIN(core)

CriticalSection::CriticalSection(void)
{
    InitializeCriticalSection(&cs_);
}

CriticalSection::CriticalSection(uint32_t spinCount)
{
    if (!InitializeCriticalSectionAndSpinCount(&cs_, spinCount)) {
        lastError::Description Dsc;
        X_ERROR("CriticalSection", "Failed to create critical section with spin count. Err: %s",
            lastError::ToString(Dsc));
    }
}

CriticalSection::~CriticalSection(void)
{
    DeleteCriticalSection(&cs_);
}

void CriticalSection::Enter(void)
{
    EnterCriticalSection(&cs_);
}

bool CriticalSection::TryEnter(void)
{
    return TryEnterCriticalSection(&cs_) != 0;
}

void CriticalSection::Leave(void)
{
    LeaveCriticalSection(&cs_);
}

void CriticalSection::SetSpinCount(uint32_t count)
{
    uint32_t previous = SetCriticalSectionSpinCount(&cs_, count);
    X_UNUSED(previous);
}

X_NAMESPACE_END