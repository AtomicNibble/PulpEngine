#pragma once

#include <EngineCommon.h>
#include <IDirectoryWatcher.h>
#include <IScriptSys.h>


#include <String\Path.h>
#include <Util\UniquePointer.h>

#include <lua.hpp>


#include "Util\Config.h"

#include "wrapper\types.h"
#include "wrapper\util.h"
#include "wrapper\state.h"
#include "wrapper\state_view.h"

#include "ScriptSys.h"

extern core::MemoryArenaBase* g_ScriptArena;


#if X_DEBUG
X_LINK_LIB("luajitd")
#else
X_LINK_LIB("luajitr")
#endif 