#pragma once

#include <EngineCommon.h>

#include "dia2.h"

#if X_64
X_LINK_LIB("amd64/diaguids.lib");
#else
X_LINK_LIB("diaguids.lib");
#endif


#ifndef TELEMETRY_SYMLIB_EXPORT

#ifdef X_LIB
#define TELEMETRY_SYMLIB_EXPORT
#else
#ifdef TELEMETRY_SYM_LIB_EXPORT
#define TELEMETRY_SYMLIB_EXPORT X_EXPORT
#else
#define TELEMETRY_SYMLIB_EXPORT X_IMPORT
#endif // TELEMETRY_SYM_LIB_EXPORT
#endif // X_LIB

#endif // TELEMETRY_SYMLIB_EXPORT


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
#endif // X_DEBUG
>
TelemSymLibArena;

extern TelemSymLibArena* g_TelemSymLibArena;
