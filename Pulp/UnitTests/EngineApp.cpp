#include "stdafx.h"
#include "EngineApp.h"

#include "ITimer.h"

#include <Debugging\DebuggerConnection.h>
#include <Platform\Module.h>



extern HINSTANCE g_hInstance;

void Error(const char* sErrorText)
{
	MessageBoxA(0, sErrorText, X_ENGINE_NAME" Start Error", MB_OK | MB_DEFAULT_DESKTOP_ONLY);
}

AssetHandler::AssetHandler(void)
{
}

AssetHandler::~AssetHandler(void)
{
}


void AssetHandler::OnAssert(const core::SourceInfo& sourceInfo)
{
	X_UNUSED(sourceInfo);

}

void AssetHandler::OnAssertVariable(const core::SourceInfo& sourceInfo)
{
	X_UNUSED(sourceInfo);

}

// -------------------------------------------------------


EngineApp::EngineApp() :
pICore_(nullptr),
hSystemHandle_(NULL)
{
}


EngineApp::~EngineApp()
{
	if (hSystemHandle_)
		core::Module::UnLoad(hSystemHandle_);
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
	params.pConsoleWnd = &Console;
	params.pCoreArena = g_arena;


#ifdef X_LIB

	pICore_ = CreateCoreInterface(params);

#else
	// load the dll.
	hSystemHandle_ = core::Module::Load(CORE_DLL_NAME);

	if (!hSystemHandle_)
	{
		Error(CORE_DLL_NAME" Loading Failed");
		return false;
	}

	CreateCoreInfterFaceFunc::Pointer pfnCreateCoreInterface =
		reinterpret_cast<CreateCoreInfterFaceFunc::Pointer>(
			core::Module::GetProc(hSystemHandle_, CORE_DLL_INITFUNC));

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

	pICore_->RegisterAssertHandler(&assertCallback_);

	LinkModule(pICore_, "UnitTest");

	if (!pICore_->IntializeLoadedConverterModule("Engine_ImgLib", "Engine_ImgLib")) {
		Error("Failed to init imgLib");
		return false;
	}

	return true;
}


int EngineApp::MainLoop()
{
	pICore_->RunGameLoop();
	pICore_->UnRegisterAssertHandler(&assertCallback_);
	pICore_->Release();
	return 0;
}

