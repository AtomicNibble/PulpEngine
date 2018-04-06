#pragma once

#include <EngineCommon.h>
#include <IFont.h>

#include "Memory\BoundsCheckingPolicies\NoBoundsChecking.h"
#include "Memory\MemoryTaggingPolicies\NoMemoryTagging.h"
#include "Memory\MemoryTrackingPolicies\NoMemoryTracking.h"

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
    FontLibArena;

extern FontLibArena* g_FontLibArena;

#ifdef X_LIB
#define FONTLIB_EXPORT
#else
#ifdef FONT_LIB_EXPORT
#define FONTLIB_EXPORT X_EXPORT
#else
#define FONTLIB_EXPORT X_IMPORT
#endif // !FONT_LIB_EXPORT
#endif // X_LIB

#if X_DEBUG
X_LINK_LIB("freetype265d");
#else
X_LINK_LIB("freetype265");
#endif // !X_DEBUG
