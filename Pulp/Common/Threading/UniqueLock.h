#pragma once

X_NAMESPACE_BEGIN(core)

struct adopt_lock_t
{
};

struct try_to_lock_t
{
};

constexpr adopt_lock_t adopt_lock{};
constexpr try_to_lock_t try_to_lock{};

template<class LockT>
class UniqueLock
{
public:
    X_INLINE UniqueLock();
    X_INLINE explicit UniqueLock(LockT& lock);
    X_INLINE UniqueLock(LockT& lock, adopt_lock_t);
    X_INLINE UniqueLock(LockT& lock, try_to_lock_t);

    X_INLINE UniqueLock(UniqueLock&& oth);
    X_INLINE UniqueLock& operator=(UniqueLock&& oth);
    X_INLINE ~UniqueLock();

    X_INLINE void Enter(void);
    X_INLINE bool TryEnter(void);
    X_INLINE void Leave(void);

    X_INLINE bool ownsLock(void) const;
    X_INLINE explicit operator bool() const;

    X_INLINE void swap(UniqueLock& oth);

private:
    X_NO_COPY(UniqueLock);
    X_NO_ASSIGN(UniqueLock);

private:
    void validate(void) const;

private:
    LockT* pLock_;
    bool owns_;
};

X_NAMESPACE_END

#include "UniqueLock.inl"
