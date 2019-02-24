#pragma once

#define _LAUNCHER
#include <EngineCommon.h>

#include <Compression/LZ4.h>
#include <Time/DateTimeStamp.h>

#include <../TelemetryCommon/TelemetryCommonLib.h>
#include <../TelemetryServerLib/TelemetryServerLib.h>

// SDL
#include <SDL.h>

// imgui
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>
#include <examples/imgui_impl_sdl.h>
#include <examples/imgui_impl_opengl3.h>
#include <examples/libs/gl3w/GL/gl3w.h>


// X_LINK_ENGINE_LIB("TelemetryServerLib");

X_LINK_LIB("opengl32.lib");
X_LINK_LIB("SDL2.lib");
// X_LINK_LIB("SDL2main.lib");


X_DISABLE_WARNING(4505) // unreferenced local function has been removed

typedef core::MemoryArena<
    core::MallocFreeAllocator,
    core::SingleThreadPolicy,
#if X_ENABLE_MEMORY_DEBUG_POLICIES
    core::SimpleBoundsChecking,
    core::SimpleMemoryTracking,
    core::SimpleMemoryTagging
#else
    core::NoBoundsChecking,
    core::NoMemoryTracking,
    core::NoMemoryTagging
#endif // !X_ENABLE_MEMORY_SIMPLE_TRACKING
>
TelemetryViewerArena;

extern TelemetryViewerArena* g_arena;