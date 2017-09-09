#include "stdafx.h"
#include "EngineApp.h"


#include <Platform\Module.h>
#include <Platform\MessageBox.h>

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
	params.bLoadSymbols = false;
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

	LinkModule(pICore_, "MayaExport");
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


void EngineApp::Error(const char* pErrorText)
{
	core::msgbox::show(pErrorText,
		X_ENGINE_NAME" Start Error",
		core::msgbox::Style::Error | core::msgbox::Style::Topmost | core::msgbox::Style::DefaultDesktop,
		core::msgbox::Buttons::OK
	);
}