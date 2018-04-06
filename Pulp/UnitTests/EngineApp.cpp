#include "stdafx.h"
#include "EngineApp.h"

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
    hSystemHandle_(NULL)
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
    params.bSkipInput = true;
    params.bSkipSound = true;
    params.bVsLog = false;
    params.bConsoleLog = true;
    params.bTesting = true;
    params.bCoreOnly = true;
    params.bEnableJobSystem = false;
    params.bProfileSysEnabled = false;
    params.pConsoleWnd = &Console;
    params.pCoreArena = g_arena;

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

    LinkModule(pICore_, "UnitTest");

    if (!pICore_->IntializeLoadedConverterModule(X_ENGINE_OUTPUT_PREFIX "ImgLib", "Engine_ImgLib")) {
        Error("Failed to init imgLib");
        return false;
    }

    return true;
}

bool EngineApp::ShutDown(void)
{
    if (pICore_) {
        pICore_->UnRegisterAssertHandler(&assertCallback_);
        pICore_->Release();
    }

    pICore_ = nullptr;
    return true;
}

void EngineApp::Error(const char* pErrorText)
{
    core::msgbox::show(pErrorText,
        X_ENGINE_NAME " Start Error",
        core::msgbox::Style::Error | core::msgbox::Style::Topmost | core::msgbox::Style::DefaultDesktop,
        core::msgbox::Buttons::OK);
}