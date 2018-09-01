#pragma once

#include <EngineCommon.h>
#include <IVideo.h>

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
    VideoLibArena;

extern VideoLibArena* g_VideoLibArena;

#ifdef X_LIB
#define VIDEOLIB_EXPORT
#else
#ifdef VIDEO_LIB_EXPORT
#define VIDEOLIB_EXPORT X_EXPORT
#else
#define VIDEOLIB_EXPORT X_IMPORT
#endif // !VIDEO_LIB_EXPORT
#endif // X_LIB


// Webm
#include <common\hdr_util.h>
#include <common\indent.h>
#include <common\webm_constants.h>
#include <common\webm_endian.h>

#include <mkvparser\mkvparser.h>


#if X_DEBUG
X_LINK_LIB("libwebmd");
#else
X_LINK_LIB("libwebm");
#endif