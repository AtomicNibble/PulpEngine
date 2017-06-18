#include "stdafx.h"
#include "EngineApp.h"

#include <ITimer.h>
#include <IPhysics.h>

#include <Debugging\DebuggerConnection.h>
#include <Platform\MessageBox.h>

EngineApp::EngineApp() :
	pICore_(nullptr),
	hSystemHandle_(0)
{
}


EngineApp::~EngineApp()
{
	ShutDown();

	if (hSystemHandle_) {
		core::Module::UnLoad(hSystemHandle_);
	}

	gEnv = nullptr;
}


bool EngineApp::Init(HINSTANCE hInstance, const wchar_t* sInCmdLine, core::Console& Console)
{
	SCoreInitParams params;
	params.hInstance = hInstance;
	params.pCmdLine = sInCmdLine;
	params.bVsLog = false;
	params.bConsoleLog = true;
	params.bCoreOnly = true;
	params.pConsoleWnd = &Console;
	params.pCoreArena = g_arena;
	params.bEnableBasicConsole = true;
//	params.bFileSysWorkingDir = true;

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


	LinkModule(pICore_, "LevelBuilder");

	// we can use logging now.
	if (!pICore_->IntializeLoadedConverterModule("Engine_ModelLib", "Engine_ModelLib")) {
		X_ERROR("LvlBuilder", "Failed to init shaderLib");
		return false;
	}

	// ConvertLib
	IConverter* pConverterInstance = nullptr;

	if (!pICore_->IntializeLoadedConverterModule("engine_Physics", "Engine_PhysLib",
		&pPhysConverterMod_, &pConverterInstance))
	{
		X_ERROR("LvlBuilder", "Engine Init Failed");
		return false;
	}

	pPhysLib_ = static_cast<physics::IPhysLib*>(pConverterInstance);
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

physics::IPhysicsCooking* EngineApp::GetPhysCooking(void)
{
	X_ASSERT_NOT_NULL(pPhysLib_);
	return pPhysLib_->getCooking();
}

void EngineApp::OnAssert(const core::SourceInfo& sourceInfo)
{
	X_UNUSED(sourceInfo);

}

void EngineApp::OnAssertVariable(const core::SourceInfo& sourceInfo)
{
	X_UNUSED(sourceInfo);

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

void EngineApp::Error(const char* pErrorText)
{
	core::msgbox::show(pErrorText,
		X_ENGINE_NAME" Start Error",
		core::msgbox::Style::Error | core::msgbox::Style::Topmost | core::msgbox::Style::DefaultDesktop,
		core::msgbox::Buttons::OK
	);
}