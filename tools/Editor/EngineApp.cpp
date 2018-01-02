#include "stdafx.h"
#include "EngineApp.h"



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
		core::Module::UnLoad(hSystemHandle_);
	}
}


bool EngineApp::Init(const wchar_t* sInCmdLine)
{
	SCoreInitParams params;
	params.hInstance = g_hInstance;
	params.pCmdLine = sInCmdLine;
	params.bSkipInput = true;
	params.bSkipSound = true;
#if X_DEBUG
	params.bVsLog = true;
#else
	params.bVsLog = false;
#endif
	params.bConsoleLog = false;
	params.bTesting = false;
	params.bCoreOnly = true;
	params.bEnableBasicConsole = false;
	params.bEnableJobSystem = true; 
	params.pConsoleWnd = nullptr;
	params.pCoreArena = g_arena;
	params.bFileSysWorkingDir = true;


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

	pICore_->RegisterAssertHandler(this);

	LinkModule(pICore_, "Editor");

	// ConverterLib
	if (!pICore_->IntializeLoadedEngineModule("Engine_ConverterLib", "Engine_ConverterLib")) {
		return false;
	}

	// AssetDB
	if (!pICore_->IntializeLoadedEngineModule("Engine_AssetDB", "Engine_AssetDB")) {
		return false;
	}

	// MatLib
	if (!pICore_->IntializeLoadedConverterModule("Engine_MaterialLib", "Engine_MaterialLib")) {
		return false;
	}

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
