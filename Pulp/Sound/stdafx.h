#pragma once


#include <EngineCommon.h>

#define IPSOUND_EXPORTS

#include <ISound.h>

#include "IPSound.h"

extern core::MemoryArenaBase* g_SoundArena;


// toggle that a sound bank is required to start.
#define SOUND_INIT_BANK_REQUIRED 0