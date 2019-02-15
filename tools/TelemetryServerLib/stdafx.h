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
#define TELEMETRYSERVERLIB_EXPORT
#else
#ifdef TELEMETRYSERVER_LIB_EXPORT
#define TELEMETRYSERVERLIB_EXPORT X_EXPORT
#else
#define TELEMETRYSERVERLIB_EXPORT X_IMPORT
#endif // !TELEMETRYSERVER_LIB_EXPORT
#endif // X_LIB