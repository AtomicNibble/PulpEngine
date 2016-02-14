#include "stdafx.h"
#include "EngineApp.h"


#ifdef X_LIB
#undef X_LIB
#define X_AS_LIB
#endif // !X_LIB

#define _LAUNCHER

#include <ModuleExports.h>

#ifdef X_AS_LIB
#define X_LIB
#endif // !X_AS_LIB


X_LINK_LIB("Engine_Core");


extern HINSTANCE g_hInstance;

void Error(const char* sErrorText)
{
	MessageBoxA(0, sErrorText, X_ENGINE_NAME" Start Error", MB_OK | MB_DEFAULT_DESKTOP_ONLY);
}

EngineApp::EngineApp() :
	pICore_(nullptr),
	hSystemHandle_(NULL)
{
}


EngineApp::~EngineApp()
{
	if (hSystemHandle_) {
		PotatoFreeLibrary(hSystemHandle_);
	}
}


bool EngineApp::Init(void)
{
	if (pICore_) {
		return true; // already loaded.
	}

	SCoreInitParams params;
	params.hInstance = nullptr;
	params.pCmdLine = L"";
	params.bSkipInput = true;
	params.bSkipSound = true;
	params.bVsLog = false;
	params.bConsoleLog = false;
	params.bTesting = false;
	params.bCoreOnly = true;
	params.bEnableBasicConsole = false;
	params.bEnableJobSystem = true; // model compiler uses job system.
	params.pConsoleWnd = nullptr;
	params.pCoreArena = g_arena;

#ifdef X_LIB

	pICore_ = CreateCoreInterface(params);

#else
	// load the dll.
	hSystemHandle_ = PotatoLoadLibary(CORE_DLL_NAME);

	if (!hSystemHandle_)
	{
		Error(CORE_DLL_NAME" Loading Failed");
		return false;
	}

	CreateCoreInfterFaceFunc::Pointer pfnCreateCoreInterface =
		reinterpret_cast<CreateCoreInfterFaceFunc::Pointer>(
			PotatoGetProcAddress(hSystemHandle_, CORE_DLL_INITFUNC));

	if (!pfnCreateCoreInterface)
	{
		Error(CORE_DLL_NAME" not valid");
		return false;
	}

	pICore_ = pfnCreateCoreInterface(params);

#endif // !X_LIB

	if (!pICore_)
	{
		Error("Engine Init Failed");
		return false;
	}

	LinkModule(pICore_, "Conveter");
	return true;
}


