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
	params.bTesting = false;
	params.bCoreOnly = true;
	params.bEnableBasicConsole = false;
	params.bEnableJobSystem = false; 
	params.pConsoleWnd = &Console;
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

	pICore_->RegisterAssertHandler(this);

	LinkModule(pICore_, "Compressor");

	return true;
}

bool EngineApp::ShutDown(void)
{
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

