#include "EngineCommon.h"

#include "SimpleBoundsChecking.h"

#if X_ENABLE_MEMORY_DEBUG_POLICIES_DEFS

X_NAMESPACE_BEGIN(core)

const char* const SimpleBoundsChecking::TYPE_NAME = "SimpleBoundsChecking";

void SimpleBoundsChecking::GuardFront(void* memory)
{
    memset(memory, TOKEN_FRONT, 4);
}

void SimpleBoundsChecking::GuardBack(void* memory)
{
    memset(memory, TOKEN_BACK, 4);
}

void SimpleBoundsChecking::CheckFront(const void* memory)
{
    union
    {
        const void* as_void;
        BYTE* as_char;
    };

    as_void = memory;

    for (int32_t i = 0; i < 4; i++) {
        X_ASSERT(as_char[i] == TOKEN_FRONT, "Memory has been overwritten at address 0x%08p", memory)(i, TOKEN_FRONT);
    }
}

void SimpleBoundsChecking::CheckBack(const void* memory)
{
    union
    {
        const void* as_void;
        BYTE* as_char;
    };

    as_void = memory;

    for (int32_t i = 0; i < 4; i++) {
        X_ASSERT(as_char[i] == TOKEN_BACK, "Memory has been overwritten at address 0x%08p", memory)(i, TOKEN_FRONT);
    }
}

X_NAMESPACE_END

#endif // !X_ENABLE_MEMORY_DEBUG_POLICIES_DEFS
