#pragma once

#ifndef X_NOMEMORYTAGGING_H
#define X_NOMEMORYTAGGING_H

X_NAMESPACE_BEGIN(core)

class NoMemoryTagging
{
public:
    static const char* const TYPE_NAME;

    inline void TagAllocation(void*, size_t) const
    {
    }

    inline void TagDeallocation(void*, size_t) const
    {
    }
};

X_NAMESPACE_END

#endif
