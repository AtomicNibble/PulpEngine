
X_NAMESPACE_BEGIN(core)

SharedLock::SharedLock()
{
    InitializeSRWLock(&smtx_);
}

SharedLock::~SharedLock()
{
}

void SharedLock::Enter(void)
{
    ttTryLock(gEnv->ctx, &smtx_, "Enter");

    AcquireSRWLockExclusive(&smtx_);

    ttEndTryLock(gEnv->ctx, &smtx_, TtLockResult::Acquired);
    ttSetLockState(gEnv->ctx, &smtx_, TtLockState::Locked);
}

bool SharedLock::TryEnter(void)
{
    ttTryLock(gEnv->ctx, &smtx_, "TryEnter");

    bool res = TryAcquireSRWLockExclusive(&smtx_) != 0;

    ttEndTryLock(gEnv->ctx, &smtx_, res ? TtLockResult::Acquired : TtLockResult::Fail);
    if (res) {
        ttSetLockState(gEnv->ctx, &smtx_, TtLockState::Locked);
    }

    return res;
}

void SharedLock::Leave(void)
{
    ReleaseSRWLockExclusive(&smtx_);

    ttSetLockState(gEnv->ctx, &smtx_, TtLockState::Released);
}

void SharedLock::EnterShared(void)
{
    ttTryLock(gEnv->ctx, &smtx_, "EnterShared");

    AcquireSRWLockShared(&smtx_);

    ttEndTryLock(gEnv->ctx, &smtx_, TtLockResult::Acquired);
    ttSetLockState(gEnv->ctx, &smtx_, TtLockState::Locked);
}

bool SharedLock::TryEnterShared(void)
{
    ttTryLock(gEnv->ctx, &smtx_, "TryEnterShared");

    bool res = TryAcquireSRWLockShared(&smtx_) != 0;

    ttEndTryLock(gEnv->ctx, &smtx_, res ? TtLockResult::Acquired : TtLockResult::Fail);
    if (res) {
        ttSetLockState(gEnv->ctx, &smtx_, TtLockState::Locked);
    }

    return res;
}

void SharedLock::LeaveShared(void)
{
    ReleaseSRWLockShared(&smtx_);

    ttSetLockState(gEnv->ctx, &smtx_, TtLockState::Released);
}

SRWLOCK* SharedLock::GetNativeObject(void)
{
    return &smtx_;
}

X_NAMESPACE_END
