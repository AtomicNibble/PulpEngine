#include "stdafx.h"

#include <IConverterModule.h>
#include <Extension\XExtensionMacros.h>
#include <ModuleExports.h> // needed for gEnv


namespace
{

	core::MallocFreeAllocator g_MatLibAlloc;

} // namespace

MatLibrena* g_MatLibArena = nullptr;
