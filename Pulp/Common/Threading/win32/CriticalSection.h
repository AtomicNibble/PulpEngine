#pragma once

#ifndef X_CRITICALSECTION_H_
#define X_CRITICALSECTION_H_

X_NAMESPACE_BEGIN(core)


class CriticalSection
{
public:
    CriticalSection(void);
    explicit CriticalSection(uint32_t spinCount);

    ~CriticalSection(void);

    void Enter(void);
    bool TryEnter(void);
    void Leave(void);

    void SetSpinCount(uint32_t count);

    inline CRITICAL_SECTION* GetNativeObject(void);

    class ScopedLock
    {
    public:
        inline explicit ScopedLock(CriticalSection& criticalSection);
        inline ~ScopedLock(void);

    private:
        X_NO_COPY(ScopedLock);
        X_NO_ASSIGN(ScopedLock);

        CriticalSection& cs_;
    };

private:
    CRITICAL_SECTION cs_;
};

#include "CriticalSection.inl"

X_NAMESPACE_END

#endif // X_CRITICALSECTION_H_
