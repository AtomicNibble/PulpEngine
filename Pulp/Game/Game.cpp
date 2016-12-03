#include "stdafx.h"
#include "Game.h"
#include "EngineApp.h"

#include <IFileSys.h>

#include <String\Path.h>

HINSTANCE g_hInstance = 0;

#include <tchar.h>

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
//X_LINK_LIB("engine_RenderDx10")
X_LINK_LIB("engine_RenderDx12")
X_LINK_LIB("engine_3DEngine")
X_LINK_LIB("engine_Physics")
X_LINK_LIB("engine_GameDLL")

X_FORCE_LINK_FACTORY("XEngineModule_Input")
X_FORCE_LINK_FACTORY("XEngineModule_Font")
X_FORCE_LINK_FACTORY("XEngineModule_Script")
X_FORCE_LINK_FACTORY("XEngineModule_Sound")
X_FORCE_LINK_FACTORY("XEngineModule_3DEngine")
X_FORCE_LINK_FACTORY("XEngineModule_Physics")
X_FORCE_LINK_FACTORY("XEngineModule_Game")

X_FORCE_SYMBOL_LINK("?s_factory@XEngineModule_Render@render@Potato@@0V?$XSingletonFactory@VXEngineModule_Render@render@Potato@@@@A");
// X_FORCE_SYMBOL_LINK("?factory__@XFactory@XEngineModule_Render@render@Potato@@0V1234@A");

// some libs that w link against.
X_LINK_LIB("engine_ImgLib")
X_FORCE_LINK_FACTORY("XConverterLib_Img")

#endif // !X_LIB

void InitRootDir(void)
{
#ifdef WIN32
	WCHAR szExeFileName[_MAX_PATH] = { 0 };
	GetModuleFileNameW(GetModuleHandleW(NULL), szExeFileName, sizeof(szExeFileName));

	core::Path<wchar_t> path(szExeFileName);

	path.removeFileName();
	path.removeTrailingSlash();

	if (!SetCurrentDirectoryW(path.c_str())) {
		::MessageBoxW(0, L"Failed to set current directory", L"Error", MB_OK);
		ExitProcess(static_cast<uint32_t>(-1));
	}
#endif
}

namespace 
{
	core::MallocFreeAllocator gAlloc;

} // namespace

void* operator new(size_t sz)
{
	return gAlloc.allocate(sz, 4, 0);
}

void operator delete(void* m)
{
	if (m) {
		gAlloc.free(m);
	}
}




int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPTSTR lpCmdLine, int nCmdShow)
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
