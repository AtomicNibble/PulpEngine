#include "stdafx.h"
#include "EngineApp.h"
#include "Resource.h"

#include "ITimer.h"

#include <Debugging\DebuggerConnection.h>




extern HINSTANCE g_hInstance;

void Error(const char* sErrorText)
{
	MessageBoxA(0, sErrorText, X_ENGINE_NAME" Start Error", MB_OK | MB_DEFAULT_DESKTOP_ONLY);
}

EngineApp::EngineApp() :
	run_(true),
	pICore_(nullptr),
	hSystemHandle_(NULL)
{
}


EngineApp::~EngineApp()
{
	ShutDown();

	if (hSystemHandle_) {
		PotatoFreeLibrary(hSystemHandle_);
	}
}


bool EngineApp::Init(const wchar_t* sInCmdLine, core::Console& Console)
{
	SCoreInitParams params;
	params.hInstance = g_hInstance;
	params.pCmdLine = sInCmdLine;
	params.bSkipInput = true;
	params.bSkipSound = true;
	params.bVsLog = false;
	params.bConsoleLog = true;
	params.bTesting = true;
	params.bCoreOnly = true;
	params.bEnableBasicConsole = false;
	params.bEnableJobSystem = true; // some converters make use of the job system. 
	params.pConsoleWnd = &Console;
	params.pCoreArena = g_arena;
	params.bFileSysWorkingDir = true;


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

	pICore_->RegisterAssertHandler(this);

	LinkModule(pICore_, "AssetServer");


	// AssetDB
#if !defined(X_LIB) 
	if (!pICore_->IntializeLoadedEngineModule("Engine_AssetDB", "Engine_AssetDB")) {
		return false;
	}
#endif

	CreateIcon(0, L"Potato - AssetServer", IDI_ASSETSERVER, IDR_MENU1);
	return true;
}

bool EngineApp::ShutDown(void)
{
	this->Destory();

	if (pICore_) {
		pICore_->UnRegisterAssertHandler(this);
		pICore_->Release();
	}
	pICore_ = nullptr;
	return true;
}

void EngineApp::OnAssert(const core::SourceInfo& sourceInfo)
{
	X_UNUSED(sourceInfo);

}

void EngineApp::OnAssertVariable(const core::SourceInfo& sourceInfo)
{
	X_UNUSED(sourceInfo);

}

LRESULT EngineApp::OnTrayCmd(WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam))
	{
	case ID_MENU_EXIT:
		run_ = false;
		break;
	}

	return 0;
}


bool EngineApp::PumpMessages(void)
{
	MSG msg;
	while (GetMessageW(&msg, 0, 0, 0) > 0 && run_)
	{
		if (msg.message == WM_QUIT) {
			return false;
		}

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return true;
}


