#include "stdafx.h"
#include "Game.h"
#include "EngineApp.h"

#include <Shlwapi.h>
#include <IFileSys.h>

HINSTANCE g_hInstance = 0;

X_LINK_LIB("Shlwapi")

#define _LAUNCHER

// #undef X_LIB
#include <ModuleExports.h>


#ifdef X_LIB
struct XRegFactoryNode* g_pHeadToRegFactories = 0;

X_LINK_LIB("engine_Input")
X_LINK_LIB("engine_Font")
X_LINK_LIB("engine_Core")
X_LINK_LIB("engine_Script")
X_LINK_LIB("engine_Sound")
X_LINK_LIB("engine_RenderDx10")
X_LINK_LIB("engine_3DEngine")
X_LINK_LIB("engine_GameDLL")

X_FORCE_SYMBOL_LINK("?factory__@XFactory@XEngineModule_Input@@0V12@A")
X_FORCE_SYMBOL_LINK("?factory__@XFactory@XEngineModule_Font@@0V12@A")
X_FORCE_SYMBOL_LINK("?factory__@XFactory@XEngineModule_Script@@0V12@A")
X_FORCE_SYMBOL_LINK("?factory__@XFactory@XEngineModule_Sound@@0V12@A")
X_FORCE_SYMBOL_LINK("?factory__@XFactory@XEngineModule_Render@@0V12@A")
X_FORCE_SYMBOL_LINK("?factory__@XFactory@XEngineModule_3DEngine@@0V12@A")
X_FORCE_SYMBOL_LINK("?factory__@XFactory@XEngineModule_Game@@0V12@A")

#endif // !X_LIB

void InitRootDir()
{
#ifdef WIN32
	WCHAR szExeFileName[_MAX_PATH];
	GetModuleFileNameW(GetModuleHandle(NULL), szExeFileName, sizeof(szExeFileName));
	PathRemoveFileSpecW(szExeFileName);
	SetCurrentDirectoryW(szExeFileName);
#endif
}


core::MallocFreeAllocator goat;

void* operator new(size_t sz)
{
	return goat.allocate(sz, 4, 0);
}

void operator delete(void* m)
{
	if (m)
		goat.free(m);
}




int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	X_UNUSED(hPrevInstance);
	X_UNUSED(lpCmdLine);
	X_UNUSED(nCmdShow);
	InitRootDir();
	g_hInstance = hInstance;

	int nRes = 0;


	{ // scope it for leak tests.
		EngineApp engine;

		if (engine.Init(lpCmdLine)) {
			nRes = engine.MainLoop();
		}
	}

	return nRes;
}
