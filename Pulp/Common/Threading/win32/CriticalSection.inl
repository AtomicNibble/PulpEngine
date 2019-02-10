

inline CRITICAL_SECTION* CriticalSection::GetNativeObject(void)
{
    return &cs_;
}

inline void CriticalSection::Enter(void)
{
    EnterCriticalSection(&cs_);
}

inline bool CriticalSection::TryEnter(void)
{
    return TryEnterCriticalSection(&cs_) != 0;
}

inline void CriticalSection::Leave(void)
{
    LeaveCriticalSection(&cs_);
}