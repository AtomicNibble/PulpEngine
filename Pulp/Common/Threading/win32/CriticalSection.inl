
// ---------------------------------------------------------------------------------------------------------------------
inline CRITICAL_SECTION* CriticalSection::GetNativeObject(void)
{
    return &cs_;
}

// ---------------------------------------------------------------------------------------------------------------------
inline CriticalSection::ScopedLock::ScopedLock(CriticalSection& criticalSection) :
    cs_(criticalSection)
{
    criticalSection.Enter();
}

// ---------------------------------------------------------------------------------------------------------------------
inline CriticalSection::ScopedLock::~ScopedLock(void)
{
    cs_.Leave();
}
