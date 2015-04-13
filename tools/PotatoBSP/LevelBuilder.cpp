#include "stdafx.h"
#include "LevelBuilder.h"
#include "EngineApp.h"

#include <tchar.h>

#include <IFileSys.h>
#include <ITimer.h>

#include "MapLoader.h"
#include "BSPTypes.h"


#define _LAUNCHER
#include <ModuleExports.h>

#include <Memory\MemoryTrackingPolicies\NoMemoryTracking.h>

HINSTANCE g_hInstance = 0;

core::MemoryArenaBase* g_arena = nullptr;

typedef core::MemoryArena<
	core::MallocFreeAllocator,
	core::SingleThreadPolicy,
	core::SimpleBoundsChecking,
	//	core::SimpleMemoryTracking,
	core::NoMemoryTracking,			// allow leaks in the tests.
	core::SimpleMemoryTagging
> UnitTestArena;


#ifdef X_LIB
struct XRegFactoryNode* g_pHeadToRegFactories = 0;

// X_LINK_LIB("engine_Font")
X_LINK_LIB("engine_Core")
// X_LINK_LIB("engine_3DEngine")
X_LINK_LIB("engine_RenderNull")
// X_LINK_LIB("engine_Script")


// X_FORCE_SYMBOL_LINK("?factory__@XFactory@XEngineModule_Render@@0V12@A")
// X_FORCE_SYMBOL_LINK("?factory__@XFactory@XEngineModule_Font@@0V12@A")
// X_FORCE_SYMBOL_LINK("?factory__@XFactory@XEngineModule_Script@@0V12@A")

#endif // !X_LIB

void CompileLevel(core::Path& path);

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPTSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	g_hInstance = hInstance;
	// compile my anus.

	core::MallocFreeAllocator allocator;
	UnitTestArena arena(&allocator, "LevelBuilderArena");

	g_arena = &arena;


	core::Console Console("Level Compiler");
	Console.SetSize(150, 60, 8000);
	Console.MoveTo(10, 10);

	{
		EngineApp engine;

		// we need the engine for Assets, Logging, Profiling, FileSystem.
		if (engine.Init(lpCmdLine, Console))
		{
			core::Path name;
			name.setFileName("basic - Copy.map");
			name.setFileName("alcatraz.map");
			name.setFileName("killzone.map");
		//	name.setFileName("box.map");
			
			CompileLevel(name);

			X_LOG0("Level", "Operation Complete...");
		}
	}

//	system("PAUSE");
	return 0;
}


void CompileLevel(core::Path& path)
{
	if (core::strUtil::IsEqualCaseInsen("map", path.extension()))
	{
		X_ERROR("Map", "extension is not valid, must be .map");
		return;
	}
	core::IFileSys::fileModeFlags mode;
	mode.Set(core::IFileSys::fileMode::READ);

	core::XFileMem* pFile;

	X_LOG0("Map", "Loading: \"%s\"", path.fileName());

	core::TimeVal start = gEnv->pTimer->GetAsyncTime();

	if (pFile = gEnv->pFileSys->openFileMem(path.c_str(), mode))
	{
		mapfile::XMapFile map;
		BSPBuilder bsp;

		//	parse the map file.
		if (map.Parse(pFile->getBufferStart(),pFile->getSize()))
		{
			core::TimeVal end = gEnv->pTimer->GetAsyncTime();
			{
				X_LOG_BULLET;
				X_LOG0("Map", "Loaded: %.4fms", (end - start).GetMilliSeconds());
				X_LOG0("Map", "Nun Entities: %i", map.getNumEntities());
				X_LOG0("Map", "Nun Brushes: %i", map.getNumBrushes());
				X_LOG0("Map", "Nun Patches: %i", map.getNumPatches());
			}

			// all loaded time to get naked.
			if (bsp.LoadFromMap(&map))
			{
				if (bsp.ProcessModels())
				{
					if (bsp.save(path.fileName()))
					{
						X_LOG0("Bsp", "saved as: \"%s\"", path.fileName());
					}
					else
					{
						X_ERROR("Bsp", "Failed to save: \"%s\"", path.fileName());
					}
				}
			}


			end = gEnv->pTimer->GetAsyncTime();
			X_LOG0("Info", "Total Time: %.4fms", (end - start).GetMilliSeconds());
		}

		gEnv->pFileSys->closeFileMem(pFile);
	}
}