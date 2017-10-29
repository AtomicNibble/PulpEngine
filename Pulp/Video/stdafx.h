#pragma once

#include <EngineCommon.h>

#include <IFileSys.h>

#include <IVideo.h>

// lib vpx
#include <vpx\vpx_decoder.h>
#include <vpx\vp8dx.h>

#include <libyuv.h>

#if X_DEBUG
X_LINK_LIB("vpxmdd");
#else
X_LINK_LIB("vpxmd");
#endif // !X_DEBUG

extern core::MemoryArenaBase* g_VideoArena;
