#include "EngineCommon.h"
#include "SimpleMemoryTagging.h"

#if X_ENABLE_MEMORY_DEBUG_POLICIES_DEFS

X_NAMESPACE_BEGIN(core)

const char* const SimpleMemoryTagging::TYPE_NAME = "MemoryTag";

void SimpleMemoryTagging::TagAllocation(void* memory, size_t size)
{
    memset(memory, TAG_ALLOCATED, size);
}

void SimpleMemoryTagging::TagDeallocation(void* memory, size_t size)
{
    memset(memory, TAG_FREED, size);
}

X_NAMESPACE_END

#endif // !X_ENABLE_MEMORY_DEBUG_POLICIES_DEFS
