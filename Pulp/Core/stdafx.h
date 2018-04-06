#pragma once

#define IPCORE_EXPORTS

#include <EngineCommon.h>

#include <IInput.h>

// forward declarations for common Interfaces.
X_NAMESPACE_DECLARE(input, struct IInput);
X_NAMESPACE_DECLARE(core, struct ITimer);

extern core::MemoryArenaBase* g_coreArena;
