

inline CRITICAL_SECTION* CriticalSection::GetNativeObject(void)
{
    return &cs_;
}

inline void CriticalSection::Enter(void)
{
    ttTryLock(gEnv->ctx, &cs_, "Enter");

    EnterCriticalSection(&cs_);

    ttEndTryLock(gEnv->ctx, &cs_, TtLockResult::Acquired);
    ttSetLockState(gEnv->ctx, &cs_, TtLockState::Locked);
}

inline bool CriticalSection::TryEnter(void)
{
    ttTryLock(gEnv->ctx, &cs_, "TryEnter");

    bool res = TryEnterCriticalSection(&cs_) != 0;

    ttEndTryLock(gEnv->ctx, &cs_, res ? TtLockResult::Acquired : TtLockResult::Fail);
    if (res) {
        ttSetLockState(gEnv->ctx, &cs_, TtLockState::Locked);
    }

    return res;
}

inline void CriticalSection::Leave(void)
{
    LeaveCriticalSection(&cs_);

    ttSetLockState(gEnv->ctx, &cs_, TtLockState::Released);
}