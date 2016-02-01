#include "stdafx.h"

#include <ModuleExports.h> // needed for gEnv


namespace
{

	core::MallocFreeAllocator g_AssetDBAlloc;

} // namespace

AssetDBArena* g_AssetDBArena = nullptr;
