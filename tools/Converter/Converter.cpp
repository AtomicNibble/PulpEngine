#include "stdafx.h"
#include "Converter.h"
#include "EngineApp.h"

#include <Memory\MemoryTrackingPolicies\NoMemoryTracking.h>
#include <Memory\ThreadPolicies\MultiThreadPolicy.h>

#include <Platform\Console.h>

#define _LAUNCHER
#include <ModuleExports.h>

#include <IAnimation.h>

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


	if (engine.Init(lpCmdLine, Console))
	{
		{
			HMODULE hAnimLib = PotatoLoadLibary("engine_AnimLib");
			if (hAnimLib)
			{
				typedef core::traits::Function<anim::IAnimLib* (ICore *)> AnimLibInter;

				AnimLibInter::Pointer pFunc = reinterpret_cast<AnimLibInter::Pointer>(PotatoGetProcAddress(hAnimLib, "CreateInterface"));
				if (pFunc)
				{
					anim::IAnimLib* pLib = pFunc(gEnv->pCore);

					const char* animInter = "C:\\Users\\WinCat\\Documents\\code\\WinCat\\engine\\potatoengine\\game_folder\\mod\\anims\\anim_test_01.anim_inter";
					const char* pModel = "C:\\Users\\WinCat\\Documents\\code\\WinCat\\engine\\potatoengine\\game_folder\\mod\\models\\anim_test.model";
					const char* pDest = "K:\\anim_test_01";

					pLib->ConvertAnim(animInter, pModel, pDest);
				}
			}
		}
		system("PAUSE");
	}

    return 0;
}

