#pragma once

#ifndef X_SIMPLEMEMORYTAGGING_H_
#define X_SIMPLEMEMORYTAGGING_H_

#if X_ENABLE_MEMORY_DEBUG_POLICIES_DEFS

X_NAMESPACE_BEGIN(core)

class SimpleMemoryTagging
{
public:
    static const char* const TYPE_NAME;

    void TagAllocation(void* memory, size_t size);
    void TagDeallocation(void* memory, size_t size);

private:
    static const int TAG_ALLOCATED = 0xBB;
    static const int TAG_FREED = 0xEE;
};

X_NAMESPACE_END

#endif // X_ENABLE_MEMORY_DEBUG_POLICIES_DEFS

#endif // X_SIMPLEMEMORYTAGGING_H_
