#include "stdafx.h"
#include "LvlBuilder.h"
#include "EngineApp.h"

#include <tchar.h>

#include <IFileSys.h>
#include <ITimer.h>
#include <IConsole.h>

#include "MapLoader.h"
#include "BSPTypes.h"

#define _LAUNCHER
#include <ModuleExports.h>

#include <Time\StopWatch.h>

#include <Memory\MemoryTrackingPolicies\NoMemoryTracking.h>
#include <Memory\AllocationPolicies\GrowingPoolAllocator.h>
#include <Memory\VirtualMem.h>

HINSTANCE g_hInstance = 0;

core::MemoryArenaBase* g_arena = nullptr;
core::MemoryArenaBase* g_bspFaceAllocator = nullptr;
core::MemoryArenaBase* g_bspNodeAllocator = nullptr;


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
> LvlBuilderArena;

typedef core::MemoryArena<
	core::GrowingPoolAllocator,
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
> PoolArena;


X_LINK_LIB("engine_MaterialLib");
X_LINK_LIB("engine_ModelLib");


#ifdef X_LIB
struct XRegFactoryNode* g_pHeadToRegFactories = 0;

// X_LINK_LIB("engine_Font")
X_LINK_LIB("engine_Core")
X_LINK_LIB("engine_3DEngine")
X_LINK_LIB("engine_RenderNull")
// X_LINK_LIB("engine_Script")


// X_FORCE_SYMBOL_LINK("?factory__@XFactory@XEngineModule_Render@@0V12@A")
// X_FORCE_SYMBOL_LINK("?factory__@XFactory@XEngineModule_Font@@0V12@A")
// X_FORCE_SYMBOL_LINK("?factory__@XFactory@XEngineModule_Script@@0V12@A")

#endif // !X_LIB

void CompileLevel(core::Path<char>& path);

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPTSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);

	g_hInstance = hInstance;
	// compile my anus.

	{
		core::MallocFreeAllocator allocator;
		LvlBuilderArena arena(&allocator, "LevelBuilderArena");

		g_arena = &arena;

		// init the pool allocators.
		core::GrowingPoolAllocator bspFaceAllocator(
			sizeof(bspFace)* (1 << 20),
			core::VirtualMem::GetPageSize() * 64,
			0,
			sizeof(bspFace),
			X_ALIGN_OF(bspFace),
			0
			);

		PoolArena bspFaceArena(&bspFaceAllocator, "bspFaceArena");

		core::GrowingPoolAllocator bspNodeAllocator(
			sizeof(bspNode)* (1 << 20),
			core::VirtualMem::GetPageSize() * 64,
			0,
			sizeof(bspNode),
			X_ALIGN_OF(bspNode),
			0
			);

		PoolArena bspNodeArena(&bspNodeAllocator, "bspNodeAllocator");

		// set the pointers.
		g_bspFaceAllocator = &bspFaceArena;
		g_bspNodeAllocator = &bspNodeArena;


		core::Console Console(L"Level Compiler");
		Console.SetSize(150, 60, 8000);
		Console.MoveTo(10, 10);

		{
			EngineApp engine;

			// we need the engine for Assets, Logging, Profiling, FileSystem.
			if (engine.Init(lpCmdLine, Console))
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
				
				CompileLevel(name);

				X_LOG0("Level", "Operation Complete...");
			}
		}
	}

	g_arena = nullptr;
	g_bspFaceAllocator = nullptr;
	g_bspNodeAllocator = nullptr;

	return 0;
}


void CompileLevel(core::Path<char>& path)
{
	core::ITimer* pTimer = gEnv->pTimer;

	if (core::strUtil::IsEqualCaseInsen("map", path.extension()))
	{
		X_ERROR("Map", "extension is not valid, must be .map");
		return;
	}
	core::IFileSys::fileModeFlags mode;
	mode.Set(core::IFileSys::fileMode::READ);

	core::XFileMem* pFile;

	X_LOG0("Map", "Loading: \"%s\"", path.fileName());

	core::StopWatch stopwatch;

	if (pFile = gEnv->pFileSys->openFileMem(path.c_str(), mode))
	{
		mapfile::XMapFile map;
		LvlBuilder lvl;

		//	parse the map file.
		if (map.Parse(pFile->getBufferStart(), safe_static_cast<size_t, uint64_t>(pFile->getSize())))
		{
			core::TimeVal elapsed = stopwatch.GetTimeVal();
			{
				X_LOG_BULLET;
				X_LOG0("Map", "Loaded: ^6%.4fms", elapsed.GetMilliSeconds());
				X_LOG0("Map", "Num Entities: ^8%" PRIuS, map.getNumEntities());
				X_LOG0("Map", "Num Brushes: ^8%" PRIuS, map.getNumBrushes());
				X_LOG0("Map", "Num Patches: ^8%" PRIuS, map.getNumPatches());
			}

			// all loaded time to get naked.
			if (lvl.LoadFromMap(&map))
			{
				if (lvl.ProcessModels())
				{
					core::StopWatch timer;

					if (lvl.save(path.fileName()))
					{
						core::TimeVal saveElapsed = timer.GetTimeVal();

						X_LOG0("Level", "Success. saved in: ^6%gms",
							saveElapsed.GetMilliSeconds());
					//	X_LOG0("Level", "saved as: \"%s\"", path.fileName());
					}
					else
					{
						X_ERROR("Level", "Failed to save: \"%s\"", path.fileName());
					}
				}
			}


			elapsed = stopwatch.GetTimeVal();
			X_LOG0("Info", "Total Time: ^6%.4fms", elapsed.GetMilliSeconds());
		}
		else
		{
			X_ERROR("Map", "Failed to parse map file");
		}

		gEnv->pFileSys->closeFileMem(pFile);
	}
}