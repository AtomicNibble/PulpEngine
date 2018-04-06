#include "stdafx.h"
#include "EngineApp.h"

#include "ITimer.h"

#include <Debugging\DebuggerConnection.h>
#include "Platform\Window.h"
#include <Platform\Module.h>
#include <Platform\MessageBox.h>

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
    hSystemHandle_(nullptr),
    allocator_()
//	arena_(&allocator_, "CoreArena")
{
    void* pMem = allocator_.allocate(sizeof(CoreArena),
        X_ALIGN_OF(CoreArena), 0);

    pArena_ = new (pMem) CoreArena(&allocator_, "CoreArena");
}

EngineApp::~EngineApp()
{
    // core allocator needs to close before this for leak reporting.
    core::Mem::Destruct(pArena_);
    allocator_.free(pArena_);

    if (hSystemHandle_) {
        core::Module::UnLoad(hSystemHandle_);
    }

    gEnv = nullptr;
}

bool EngineApp::Init(HINSTANCE hInstance, const wchar_t* sInCmdLine)
{
    X_UNUSED(sInCmdLine);

    SCoreInitParams params;
    params.pCmdLine = sInCmdLine;
    params.hInstance = hInstance;
    params.bSkipInput = false;
    params.bEnableNetowrking = true;
    params.bEnableVideo = true;
    params.bProfileSysEnabled = true;
    params.pCoreArena = pArena_;

    // enable loggers
#if 0
	params.bVsLog = false;
	params.bConsoleLog = false;
#else
    params.bVsLog = core::debugging::IsDebuggerConnected();
    params.bConsoleLog = true;
#endif

#ifdef X_LIB

    pICore_ = CreateCoreInterface(params);

#else
    // load the dll.
    hSystemHandle_ = core::Module::Load(CORE_DLL_NAME);

    if (!hSystemHandle_) {
        Error(CORE_DLL_NAME " Loading Failed");
        return false;
    }

    CreateCoreInfterFaceFunc::Pointer pfnCreateCoreInterface = reinterpret_cast<CreateCoreInfterFaceFunc::Pointer>(
        core::Module::GetProc(hSystemHandle_, CORE_DLL_INITFUNC));

    if (!pfnCreateCoreInterface) {
        Error(CORE_DLL_NAME " not valid");
        return false;
    }

    pICore_ = pfnCreateCoreInterface(params);

#endif // !X_LIB

    if (!pICore_) {
        Error("Engine Init Failed");
        return false;
    }

    pICore_->RegisterAssertHandler(&assertCallback_);

    LinkModule(pICore_, "Game");

    return true;
}

LRESULT CALLBACK EngineApp::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_CLOSE:
            return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

void EngineApp::Error(const char* pErrorText)
{
    core::msgbox::show(pErrorText,
        X_ENGINE_NAME " Start Error",
        core::msgbox::Style::Error | core::msgbox::Style::Topmost | core::msgbox::Style::DefaultDesktop,
        core::msgbox::Buttons::OK);
}

bool EngineApp::PumpMessages()
{
    MSG msg;
    while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT)
            return false;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
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
