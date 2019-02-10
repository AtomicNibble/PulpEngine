#pragma once

#ifndef X_CRITICALSECTION_H_
#define X_CRITICALSECTION_H_

#include "Threading/ScopedLock.h"

X_NAMESPACE_BEGIN(core)


class CriticalSection
{
public:
    typedef ScopedLock<CriticalSection> ScopedLock;

public:
    CriticalSection(void);
    explicit CriticalSection(uint32_t spinCount);

    ~CriticalSection(void);

    inline void Enter(void);
    inline bool TryEnter(void);
    inline void Leave(void);

    void SetSpinCount(uint32_t count);

    inline CRITICAL_SECTION* GetNativeObject(void);

private:
    CRITICAL_SECTION cs_;
};

#include "CriticalSection.inl"

X_NAMESPACE_END

#endif // X_CRITICALSECTION_H_
