#pragma once

#ifndef X_NOBOUNDSCHECKING_H_
#define X_NOBOUNDSCHECKING_H_

X_NAMESPACE_BEGIN(core)


class NoBoundsChecking
{
public:
    static const char* const TYPE_NAME;

    static const size_t SIZE_FRONT = 0;
    static const size_t SIZE_BACK = 0;

    inline void GuardFront(void*) const
    {
    }

    inline void GuardBack(void*) const
    {
    }

    inline void CheckFront(const void*) const
    {
    }

    inline void CheckBack(const void*) const
    {
    }
};

X_NAMESPACE_END

#endif
