#pragma once

#ifndef X_NOMEMORYTRACKING__H
#define X_NOMEMORYTRACKING__H

X_NAMESPACE_BEGIN(core)

class NoMemoryTracking
{
public:
    static const char* const TYPE_NAME;

    static const size_t PER_ALLOCATION_OVERHEAD = 0;

    inline void OnAllocation(void*, size_t, size_t, size_t, size_t X_MEM_HUMAN_IDS_CB(const char*) X_MEM_HUMAN_IDS_CB(const char*) X_SOURCE_INFO_MEM_CB(const SourceInfo&), const char*) const
    {
    }

    inline void OnDeallocation(void*) const
    {
    }
};

X_NAMESPACE_END

#endif // X_NOMEMORYTRACKING__H
