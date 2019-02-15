#pragma once

#include <EngineCommon.h>


typedef core::MemoryArena<
    core::MallocFreeAllocator,
    core::SingleThreadPolicy,
#if X_DEBUG
    core::SimpleBoundsChecking,
    core::SimpleMemoryTracking,
    core::SimpleMemoryTagging
#else
    core::NoBoundsChecking,
    core::NoMemoryTracking,
    core::NoMemoryTagging
#endif // !X_DEBUG
>
TelemSrvLibArena;

extern TelemSrvLibArena* g_TelemSrvLibArena;

#ifdef X_LIB
#define TELEM_SRV_EXPORT
#else
#define TELEM_SRV_EXPORT X_EXPORT
#endif // X_LIB