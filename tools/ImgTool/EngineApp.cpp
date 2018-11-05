#include "stdafx.h"
#include "EngineApp.h"

#include "ITimer.h"

#include <Debugging\DebuggerConnection.h>
#include <Platform\Module.h>
#include <Platform\MessageBox.h>

EngineApp::EngineApp() :
    hSystemHandle_(NULL),
    pICore_(nullptr)
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

bool EngineApp::Init(HINSTANCE hInstance, core::MemoryArenaBase* arena, const wchar_t* pInCmdLine)
{
    CoreInitParams params;
    params.hInstance = hInstance;
    params.pCmdLine = pInCmdLine;
    params.bSkipInput = true;
    params.bSkipSound = true;
    params.bVsLog = false;
    params.bConsoleLog = true;
    params.bTesting = false;
    params.bCoreOnly = true;
    params.bEnableBasicConsole = false;
    params.bEnableJobSystem = false;
    params.pCoreArena = arena;
    params.bFileSysWorkingDir = true;
    params.bPauseShutdown = false;
    params.consoleDesc.pTitle = X_ENGINE_NAME_W L" - ImgTool";

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

    LinkModule(pICore_, "ImgToolor");

    if (!pICore_->IntializeLoadedConverterModule(X_ENGINE_OUTPUT_PREFIX "ImgLib", "Engine_ImgLib")) {
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

void EngineApp::Error(const char* pErrorText)
{
    core::msgbox::show(pErrorText,
        X_ENGINE_NAME " Start Error",
        core::msgbox::Style::Error | core::msgbox::Style::Topmost | core::msgbox::Style::DefaultDesktop,
        core::msgbox::Buttons::OK);
}