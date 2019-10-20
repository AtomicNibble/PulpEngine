

inline CRITICAL_SECTION* CriticalSection::GetNativeObject(void)
{
    return &cs_;
}

inline void CriticalSection::Enter(void)
{
    ttTryLock(gEnv->ctx, &cs_, "Enter");

    EnterCriticalSection(&cs_);

    ttEndTryLock(gEnv->ctx, &cs_, TtLockResultAcquired);
    ttSetLockState(gEnv->ctx, &cs_, TtLockStateLocked);
}

inline bool CriticalSection::TryEnter(void)
{
    ttTryLock(gEnv->ctx, &cs_, "TryEnter");

    bool res = TryEnterCriticalSection(&cs_) != 0;

    ttEndTryLock(gEnv->ctx, &cs_, res ? TtLockResultAcquired : TtLockResultFail);
    if (res) {
        ttSetLockState(gEnv->ctx, &cs_, TtLockStateLocked);
    }

    return res;
}

inline void CriticalSection::Leave(void)
{
    LeaveCriticalSection(&cs_);

    ttSetLockState(gEnv->ctx, &cs_, TtLockStateReleased);
}
