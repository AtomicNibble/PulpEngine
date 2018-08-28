#pragma once

#include <EngineCommon.h>

#include <IFileSys.h>

#include <IVideo.h>

// lib vpx
#include <vpx\vpx_decoder.h>
#include <vpx\vp8dx.h>

#include <libyuv.h>

// vorbis / ogg
#include <ogg\ogg.h>
#include <vorbis\codec.h>

#if X_DEBUG
X_LINK_LIB("vpxmdd");
X_LINK_LIB("libvorbis_staticd");
X_LINK_LIB("libogg_staticd");
#else
X_LINK_LIB("vpxmd");
X_LINK_LIB("libvorbis_static");
X_LINK_LIB("libogg_static");
#endif // !X_DEBUG

extern core::MemoryArenaBase* g_VideoArena;
