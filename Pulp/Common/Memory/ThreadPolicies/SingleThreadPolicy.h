#pragma once

#ifndef X_SINGLETHREADPOLICY_H
#define X_SINGLETHREADPOLICY_H

X_NAMESPACE_BEGIN(core)


class SingleThreadPolicy
{
public:
    static const char* const TYPE_NAME;
    static const bool IS_THREAD_SAFE = false;

    inline void Enter(void) const
    {
    }

    inline void Leave(void) const
    {
    }
};

X_NAMESPACE_END

#endif
