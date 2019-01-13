#pragma once

#ifndef X_MULTITHREADPOLICY_H
#define X_MULTITHREADPOLICY_H

X_NAMESPACE_BEGIN(core)


template<class SynchronizationPrimitive>
class MultiThreadPolicy
{
public:
    static const char* const TYPE_NAME;
    static const bool IS_THREAD_SAFE = true;

    inline void Enter(void);
    inline void Leave(void);

private:
    SynchronizationPrimitive primitive_;
};

#include "MultiThreadPolicy.inl"

X_NAMESPACE_END

#endif
