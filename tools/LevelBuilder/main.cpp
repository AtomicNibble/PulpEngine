#include "stdafx.h"
#include "EngineApp.h"

#include <IConsole.h>
#include "Compiler.h"

#define _LAUNCHER
#include <ModuleExports.h>


#include <Memory\MemoryTrackingPolicies\NoMemoryTracking.h>
#include <Memory\MemoryTrackingPolicies\FullMemoryTracking.h>
#include <Memory\AllocationPolicies\GrowingPoolAllocator.h>
#include <Memory\VirtualMem.h>

X_NAMESPACE_BEGIN(lvl)

core::MemoryArenaBase* g_arena = nullptr;

X_NAMESPACE_END

namespace
{

	typedef core::MemoryArena<
		core::MallocFreeAllocator,
		core::SingleThreadPolicy,
	#if X_ENABLE_MEMORY_DEBUG_POLICIES
		core::SimpleBoundsChecking,
		core::FullMemoryTracking,
		core::SimpleMemoryTagging
	#else
		core::NoBoundsChecking,
		core::NoMemoryTracking,
		core::NoMemoryTagging
	#endif // !X_ENABLE_MEMORY_SIMPLE_TRACKING
	> LvlBuilderArena;

} // namespace

X_LINK_LIB("engine_MaterialLib");
X_LINK_LIB("engine_ModelLib");
X_LINK_LIB("engine_Physics");

#ifdef X_LIB
struct XRegFactoryNode* g_pHeadToRegFactories = nullptr;

// X_LINK_LIB("engine_Font")
X_LINK_LIB("engine_Core")
X_LINK_LIB("engine_RenderNull")
X_LINK_LIB("engine_Physics");

X_FORCE_LINK_FACTORY("XConverterLib_Material")
X_FORCE_LINK_FACTORY("XConverterLib_Model")
X_FORCE_LINK_FACTORY("XConverterLib_Phys")


#endif // !X_LIB


int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPTSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);

	// compile my anus.
	int res = -1;

	{
		core::MallocFreeAllocator allocator;
		LvlBuilderArena arena(&allocator, "LevelBuilderArena");

		lvl::g_arena = &arena;

		core::Console Console(X_WIDEN(X_ENGINE_NAME) L" - Level Compiler");
		Console.SetSize(150, 60, 8000);
		Console.MoveTo(10, 10);

		{
			EngineApp engine;

			// we need the engine for Assets, Logging, Profiling, FileSystem.
			if (engine.Init(hInstance, lpCmdLine, &arena, Console))
			{
				{
					core::ICVar* pLogVerbosity = gEnv->pConsole->GetCVar("log_verbosity");
					X_ASSERT_NOT_NULL(pLogVerbosity);
					pLogVerbosity->Set(0);
				}		

				core::Path<char> name;
				name.set("map_source\\");
				name.setFileName("basic - Copy.map");
				name.setFileName("alcatraz.map");
				name.setFileName("killzone.map");
				name.setFileName("box.map");
				name.setFileName("boxmap.map");
				name.setFileName("box2.map");
				name.setFileName("box3.map");
				name.setFileName("box4.map");
				name.setFileName("boxmap.map");
				name.setFileName("portal_test.map");
				name.setFileName("entity_test.map");
				name.setFileName("physics_test.map");
				
				lvl::Compiler comp(&arena, engine.GetPhysCooking());

				if (comp.init())
				{
					if (comp.compileLevel(name))
					{
						res = 0;
					}
				}

				X_LOG0("Level", "Operation Complete...");
			}

			lvl::g_arena = nullptr;
		}
	}

	return res;
}


