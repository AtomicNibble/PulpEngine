#include "stdafx.h"
#include "Game.h"
#include "EngineApp.h"

#include <IFileSys.h>

#include <String\Path.h>
#include <Platform\MessageBox.h>

#include <tchar.h>

#define _LAUNCHER

// #undef X_LIB
#include <ModuleExports.h>


#ifdef X_LIB
struct XRegFactoryNode* g_pHeadToRegFactories = nullptr;

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
X_LINK_LIB("engine_Network")
X_LINK_LIB("engine_Video")

X_FORCE_LINK_FACTORY("XEngineModule_Input")
X_FORCE_LINK_FACTORY("XEngineModule_Font")
X_FORCE_LINK_FACTORY("XEngineModule_Script")
X_FORCE_LINK_FACTORY("XEngineModule_Sound")
X_FORCE_LINK_FACTORY("XEngineModule_3DEngine")
X_FORCE_LINK_FACTORY("XEngineModule_Physics")
X_FORCE_LINK_FACTORY("XEngineModule_Game")
X_FORCE_LINK_FACTORY("XEngineModule_Network")
X_FORCE_LINK_FACTORY("XEngineModule_Video")

X_FORCE_SYMBOL_LINK("?s_factory@XEngineModule_Render@render@Potato@@0V?$XSingletonFactory@VXEngineModule_Render@render@Potato@@@@A");
// X_FORCE_SYMBOL_LINK("?factory__@XFactory@XEngineModule_Render@render@Potato@@0V1234@A");

// some libs that w link against.
X_LINK_LIB("engine_ImgLib")
X_LINK_LIB("engine_ShaderLib")
X_LINK_LIB("engine_ModelLib")
X_LINK_LIB("engine_AnimLib")

X_FORCE_LINK_FACTORY("XConverterLib_Img")
X_FORCE_LINK_FACTORY("XConverterLib_Shader")
X_FORCE_LINK_FACTORY("XConverterLib_Model")
X_FORCE_LINK_FACTORY("XConverterLib_Anim")


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
		core::msgbox::show("Failed to set current directory",
			X_ENGINE_NAME " Fatal Error",
			core::msgbox::Style::Error | core::msgbox::Style::Topmost | core::msgbox::Style::DefaultDesktop,
			core::msgbox::Buttons::OK);

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
	return gAlloc.allocate(sz, sizeof(uintptr_t), 0);
}

void* operator new[](size_t sz)
{
	return gAlloc.allocate(sz, sizeof(uintptr_t), 0);
}

void operator delete(void* m)
{
	if (m) {
		gAlloc.free(m);
	}
}

void operator delete[](void* m)
{
	if (m) {
		gAlloc.free(m);
	}
}

void operator delete(void* m, size_t sz)
{
	if (m) {
		gAlloc.free(m, sz);
	}
}

void operator delete[](void* m, size_t sz)
{
	if (m) {
		gAlloc.free(m, sz);
	}
}





int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPTSTR lpCmdLine, int nCmdShow)
{
	X_UNUSED(hPrevInstance);
	X_UNUSED(lpCmdLine);
	X_UNUSED(nCmdShow);
	InitRootDir();

	int nRes = 0;

	{ // scope it for leak tests.
		EngineApp engine;

		if (engine.Init(hInstance, lpCmdLine)) {
			nRes = engine.MainLoop();
		}
	}

	return nRes;
}
