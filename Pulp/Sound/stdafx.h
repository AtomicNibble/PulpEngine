#pragma once

#include <EngineCommon.h>

#define IPSOUND_EXPORTS

#include <ISound.h>

#include "IPSound.h"

extern core::MemoryArenaBase* g_SoundArena;

X_DISABLE_WARNING(4505)

// Sound Engine
#include <AK/SoundEngine/Common/AkSoundEngine.h>
#include <AK/SoundEngine/Common/AkModule.h>
#include <AK/SoundEngine/Common/AkStreamMgrModule.h>

// Music engine
#include <AK/MusicEngine/Common/AkMusicEngine.h>

X_ENABLE_WARNING(4505)

#include "Util\Constants.h"
#include "Util\TypeHelpers.h"
#include "Util\AkResult.h"
