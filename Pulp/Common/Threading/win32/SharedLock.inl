
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
    AcquireSRWLockExclusive(&smtx_);
}

bool SharedLock::TryEnter(void)
{
    return TryAcquireSRWLockExclusive(&smtx_) != 0;
}

void SharedLock::Leave(void)
{
    ReleaseSRWLockExclusive(&smtx_);
}

void SharedLock::EnterShared(void)
{
    AcquireSRWLockShared(&smtx_);
}

bool SharedLock::TryEnterShared(void)
{
    return TryAcquireSRWLockShared(&smtx_) != 0;
}

void SharedLock::LeaveShared(void)
{
    ReleaseSRWLockShared(&smtx_);
}

SRWLOCK* SharedLock::GetNativeObject(void)
{
    return &smtx_;
}

X_NAMESPACE_END
