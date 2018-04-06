#include "stdafx.h"
#include "EngineApp.h"

#include "ITimer.h"

#include <Debugging\DebuggerConnection.h>
#include <Platform\Module.h>
#include <Platform\MessageBox.h>
#include <Platform\Console.h>

extern HINSTANCE g_hInstance;

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
    params.bEnableJobSystem = true; // some converters make use of the job system.
    params.pConsoleWnd = &Console;
    params.pCoreArena = g_arena;
    params.bFileSysWorkingDir = true;

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

    pICore_->RegisterAssertHandler(this);

    LinkModule(pICore_, "AssetClientServer");

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
