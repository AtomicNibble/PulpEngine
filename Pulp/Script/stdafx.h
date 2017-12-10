#pragma once

#include <EngineCommon.h>

#include <lua.hpp>

#if X_DEBUG
X_LINK_LIB("luajitd")
#else
X_LINK_LIB("luajitr")
#endif 


#include <IScriptSys.h>
#include "ScriptSys.h"


#include "wrapper\types.h"
#include "wrapper\state.h"

extern core::MemoryArenaBase* g_ScriptArena;
