
#pragma once

#include <EngineCommon.h>

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
    ImgLibArena;

extern ImgLibArena* g_ImgLibArena;

#ifdef X_LIB
#define IMGLIB_EXPORT
#else
#ifdef IMG_LIB_EXPORT
#define IMGLIB_EXPORT X_EXPORT
#else
#define IMGLIB_EXPORT X_IMPORT
#endif // !IMG_LIB_EXPORT
#endif // X_LIB

namespace ispc
{
#include <../../3rdparty/source/ispc_texcomp/ispc_texcomp.h>

} // namespace ispc

X_LINK_LIB("ispc_texcomp");