#pragma once

#include <EngineCommon.h>

#include <String\Path.h>
#include <IDirectoryWatcher.h>

#include <lua.hpp>

#include <IScriptSys.h>
#include "ScriptSys.h"

#include "wrapper\types.h"
#include "wrapper\util.h"
#include "wrapper\state.h"
#include "wrapper\state_view.h"

extern core::MemoryArenaBase* g_ScriptArena;


#if X_DEBUG
X_LINK_LIB("luajitd")
#else
X_LINK_LIB("luajitr")
#endif 