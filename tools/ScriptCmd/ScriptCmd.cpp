#include "stdafx.h"
#include "ScriptCmd.h"
#include "EngineApp.h"

#define _LAUNCHER
#include <ModuleExports.h>

#include <Platform\Console.h>

#include <String\HumanSize.h>
#include <String\HumanDuration.h>

#include <Time\StopWatch.h>

#include <Util\UniquePointer.h>

#include <ICompression.h>
#include <IFileSys.h>

#ifdef X_LIB

struct XRegFactoryNode* g_pHeadToRegFactories = nullptr;

X_LINK_LIB("engine_Core")

#endif // !X_LIB

namespace
{

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
	> ScriptCmdArena;

	

} // namespace 

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	core::Console Console(X_WIDEN(X_ENGINE_NAME) L" - ScriptCmd");
	Console.RedirectSTD();
	Console.SetSize(100, 40, 2000);
	Console.MoveTo(10, 10);

	core::MallocFreeAllocator allocator;
	ScriptCmdArena arena(&allocator, "ScriptCmdArena");

	EngineApp app;

	int res = -1;

	if (!app.Init(hInstance, &arena, lpCmdLine, Console)) {
		return -1;
	}

	return res;
}
