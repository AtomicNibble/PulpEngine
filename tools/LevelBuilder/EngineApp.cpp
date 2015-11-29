#include "stdafx.h"
#include "EngineApp.h"

#include "ITimer.h"

#include <Debugging\DebuggerConnection.h>



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
	if (pICore_)
		pICore_->Release();

	if (hSystemHandle_)
		GoatFreeLibrary(hSystemHandle_);
}


bool EngineApp::Init(const wchar_t* sInCmdLine, core::Console& Console)
{
	SCoreInitParams params;
	params.hInstance = g_hInstance;
	params.bVsLog = false;
	params.bConsoleLog = true;
	params.bCoreOnly = true;
	params.pConsoleWnd = &Console;
	params.pCoreArena = g_arena;
	params.bEnableBasicConsole = true;

#ifdef X_LIB

	pICore_ = CreateCoreInterface(params);

#else
	// load the dll.
	hSystemHandle_ = GoatLoadLibary(CORE_DLL_NAME);

	if (!hSystemHandle_)
	{
		Error(CORE_DLL_NAME" Loading Failed");
		return false;
	}

	PFNCREATECOREINTERFACE pfnCreateCoreInterface =
		(PFNCREATECOREINTERFACE)GoatGetProcAddress(hSystemHandle_, CORE_DLL_INITFUNC);

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

	LinkModule(pICore_, "PotatoBSP");

	return true;
}

LRESULT CALLBACK EngineApp::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_CLOSE:
		return 0;

	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}


