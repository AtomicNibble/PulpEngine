#include "stdafx.h"
#include "EngineApp.h"

#include "ITimer.h"

#include <Debugging\DebuggerConnection.h>
#include "Platform\Window.h"



extern HINSTANCE g_hInstance;


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

void Error(const char* sErrorText)
{
	MessageBoxA(0, sErrorText, X_ENGINE_NAME" Start Error", MB_OK | MB_DEFAULT_DESKTOP_ONLY);
}

EngineApp::EngineApp() :
	pICore_(nullptr),
	hSystemHandle_(NULL),
	allocator_(),
	arena_(&allocator_, "CoreArena")
{
}


EngineApp::~EngineApp()
{
	if (hSystemHandle_)
		GoatFreeLibrary(hSystemHandle_);
}


bool EngineApp::Init(const char* sInCmdLine)
{
	SCoreInitParams params;
	params.hInstance = g_hInstance;
	params.bSkipInput = false;

	// enable loggers
	params.bVsLog = core::debugging::IsDebuggerConnected();
	params.bConsoleLog = true;
	params.pCoreArena = &arena_;

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

	pICore_->RegisterAssertHandler(&assertCallback_);

	LinkModule(pICore_, "Game");

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


bool EngineApp::PumpMessages()
{
	MSG msg;
	while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
	{
		if (msg.message == WM_QUIT)
			return false;
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return true;
}

#include "I3DEngine.h"

int EngineApp::MainLoop()
{
	while (pICore_->PumpMessages())
	{
		X_PROFILE_BEGIN("GameLoop", core::ProfileSubSys::GAME);
		
		pICore_->Update();
		pICore_->RenderBegin();
		pICore_->RenderEnd();

#if 0
		const float time = gEnv->pTimer->GetCurrTime();
		const float FrameTime = gEnv->pTimer->GetFrameTime();
		const float FrameRate = gEnv->pTimer->GetFrameRate();


		X_LOG0_EVERY_N(30, "Frame", "Time: %f FrameTime: %f FPS: %f",
			time,
			FrameTime,
			FrameRate);
#endif
	}

	pICore_->UnRegisterAssertHandler(&assertCallback_);
	pICore_->Release();

	return 0;
}

