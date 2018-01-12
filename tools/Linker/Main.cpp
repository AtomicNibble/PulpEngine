#include "stdafx.h"
#include "EngineApp.h"

#include <Memory\MemoryTrackingPolicies\NoMemoryTracking.h>
#include <Memory\ThreadPolicies\MultiThreadPolicy.h>

#include <String\HumanDuration.h>
#include <Platform\Console.h>
#include <Time\StopWatch.h>
#include <Hashing\Fnva1Hash.h>

#define _LAUNCHER
#include <ModuleExports.h>

#include <../LinkerLib/LinkerLib.h>
X_LINK_ENGINE_LIB("LinkerLib")
X_LINK_ENGINE_LIB("assetDB")


#ifdef X_LIB

struct XRegFactoryNode* g_pHeadToRegFactories = nullptr;

X_LINK_ENGINE_LIB("Core")
X_LINK_ENGINE_LIB("RenderNull")


X_FORCE_SYMBOL_LINK("?s_factory@XEngineModule_Render@render@Potato@@0V?$XSingletonFactory@VXEngineModule_Render@render@Potato@@@@A");
X_FORCE_LINK_FACTORY("XEngineModule_LinkerLib");


#endif // !X_LIB


typedef core::MemoryArena<
	core::MallocFreeAllocator,
	core::MultiThreadPolicy<core::Spinlock>,
#if X_ENABLE_MEMORY_DEBUG_POLICIES
	core::SimpleBoundsChecking,
	core::SimpleMemoryTracking,
	core::SimpleMemoryTagging
#else
	core::NoBoundsChecking,
	core::NoMemoryTracking,
	core::NoMemoryTagging
#endif // !X_ENABLE_MEMORY_SIMPLE_TRACKING
> LinkerArena;


namespace
{
	X_DECLARE_ENUM(LinkMode)(BUILD, META);


	bool GetMode(LinkMode::Enum& mode)
	{
		const wchar_t* pMode = gEnv->pCore->GetCommandLineArgForVarW(L"mode");
		if (pMode)
		{
			if (core::strUtil::IsEqualCaseInsen(pMode, L"build"))
			{
				mode = LinkMode::BUILD;
			}
			else if (core::strUtil::IsEqualCaseInsen(pMode, L"meta"))
			{
				mode = LinkMode::META;
			}
			else
			{
				X_ERROR("Linker", "Unknown mode: \"%ls\"", pMode);
				return false;
			}

			return true;
		}

		return false;
	}

	bool GetInputfile(core::Path<char>& name, bool slient = false)
	{
		const wchar_t* pFileName = gEnv->pCore->GetCommandLineArgForVarW(L"if");
		if (pFileName)
		{
			core::StackString512 nameNarrow(pFileName);

			name = core::Path<char>(nameNarrow.begin(), nameNarrow.end());
			return true;
		}

		X_ERROR_IF(!slient, "Linker", "missing input file");
		return false;
	}


}// namespace 


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	core::Console Console(X_WIDEN(X_ENGINE_NAME) L" - Linker");
	Console.RedirectSTD();
	Console.SetSize(90, 40, 2000);
	Console.MoveTo(10, 10);


	core::MallocFreeAllocator allocator;
	LinkerArena arena(&allocator, "LinkerArena");

	bool res = false;

	{
		EngineApp app; // needs to clear up before arena.

		if (app.Init(hInstance, &arena, lpCmdLine, Console))
		{
			assetDb::AssetDB db;

			linker::Linker linker(db, &arena);
			linker.PrintBanner();

			if (linker.Init())
			{
				LinkMode::Enum mode;

				if (!GetMode(mode)) {
					mode = LinkMode::BUILD;
				}

				core::StopWatch timer;

				if (mode == LinkMode::BUILD)
				{
					if (!linker.Build())
					{
						X_ERROR("Linker", "Failed to perform build");
					}
				}
				else if (mode == LinkMode::META)
				{
					core::Path<char> inputFile;

					if (GetInputfile(inputFile))
					{
						if (!linker.dumpMeta(inputFile))
						{
							X_ERROR("Linker", "Failed to dump meta");
						}
					}
				}

				core::HumanDuration::Str timeStr;
				X_LOG0("Linker", "Elapsed time: ^6%s", core::HumanDuration::toString(timeStr, timer.GetMilliSeconds()));

			}
			else
			{
				X_ERROR("Linker", "Failed to init linker");
			}

			Console.PressToContinue();
		}
	}

	return res ? 0 : -1;
}

