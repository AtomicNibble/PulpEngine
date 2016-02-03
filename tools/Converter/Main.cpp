#include "stdafx.h"
#include "Converter.h"
#include "EngineApp.h"

#include <Memory\MemoryTrackingPolicies\NoMemoryTracking.h>
#include <Memory\ThreadPolicies\MultiThreadPolicy.h>

#include <Platform\Console.h>

#include <String\CmdArgs.h>


#define _LAUNCHER
#include <ModuleExports.h>

#include <IAnimation.h>

#include "Converter.h"

HINSTANCE g_hInstance = 0;

#ifdef X_LIB

struct XRegFactoryNode* g_pHeadToRegFactories = 0;

X_LINK_LIB("engine_Core")
X_LINK_LIB("engine_RenderNull")

X_FORCE_SYMBOL_LINK("?factory__@XFactory@XEngineModule_Render@@0V12@A")

X_LINK_LIB("engine_ModelLib")
X_LINK_LIB("engine_AnimLib")

X_FORCE_SYMBOL_LINK("?factory__@XFactory@XConverterLib_Model@@0V12@A")
X_FORCE_SYMBOL_LINK("?factory__@XFactory@XConverterLib_Anim@@0V12@A")


#endif // !X_LIB


typedef core::MemoryArena<
	core::MallocFreeAllocator,
	core::SingleThreadPolicy,
	core::SimpleBoundsChecking,
	core::NoMemoryTracking,
	core::SimpleMemoryTagging
> ConverterArena;

core::MemoryArenaBase* g_arena = nullptr;


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	g_hInstance = hInstance;

	EngineApp engine;

	core::Console Console(L"Potato - Converter");
	Console.RedirectSTD();
	Console.SetSize(60, 40, 2000);
	Console.MoveTo(10, 10);


	core::MallocFreeAllocator allocator;
	ConverterArena arena(&allocator, "ConverterArena");
	g_arena = &arena;

	bool res = false;

	if (engine.Init(lpCmdLine, Console))
	{
		converter::Converter con;

		con.PrintBanner();

		{
			const wchar_t* pAssetType = gEnv->pCore->GetCommandLineArgForVarW(L"type");
			if (pAssetType)
			{
				core::CmdArgs<4096, wchar_t> args(lpCmdLine);

				if (core::strUtil::IsEqualCaseInsen(pAssetType, L"model"))
				{
					res = con.Convert(converter::AssetType::Model, args);
				}
				else if (core::strUtil::IsEqualCaseInsen(pAssetType, L"anim"))
				{
					res = con.Convert(converter::AssetType::Anim, args);
				}
				else
				{
					X_ERROR("Converter", "Unkown asset type: \"%ls\"", pAssetType);
				}
			}
		}

		system("PAUSE");
	}

	return res ? 0 : -1;
}

