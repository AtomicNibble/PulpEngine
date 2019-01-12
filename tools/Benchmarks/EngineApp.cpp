#include "stdafx.h"
#include "EngineApp.h"

#include <Platform\Module.h>
#include <Platform\MessageBox.h>

using namespace core::string_view_literals;

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
    hSystemHandle_(core::Module::NULL_HANDLE),
    arena_(&allocator_, "BenchMarkCoreArena")
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

bool EngineApp::Init(HINSTANCE hInstance, const wchar_t* pInCmdLine)
{
    CoreInitParams params;
    params.hInstance = hInstance;
    params.pCmdLine = pInCmdLine;
    params.bSkipInput = true;
    params.bSkipSound = true;
    params.bVsLog = false;
    params.bConsoleLog = true;
    params.bTesting = true;
    params.bCoreOnly = true;
    params.bEnableJobSystem = false;
    params.bProfileSysEnabled = false;
    params.pCoreArena = &arena_;
    params.bThreadSafeStringAlloc = false;
    params.consoleDesc.pTitle = X_ENGINE_NAME " - Benchmark Log";

#ifdef X_LIB

    pICore_ = CreateCoreInterface(params);

#else
    // load the dll.
    hSystemHandle_ = core::Module::Load(CORE_DLL_NAME);

    if (!hSystemHandle_) {
        Error(CORE_DLL_NAME " Loading Failed"_sv);
        return false;
    }

    CreateCoreInfterFaceFunc::Pointer pfnCreateCoreInterface = reinterpret_cast<CreateCoreInfterFaceFunc::Pointer>(
        core::Module::GetProc(hSystemHandle_, CORE_DLL_INITFUNC));

    if (!pfnCreateCoreInterface) {
        Error(CORE_DLL_NAME " not valid"_sv);
        return false;
    }

    pICore_ = pfnCreateCoreInterface(params);

#endif // !X_LIB

    if (!pICore_) {
        Error("Engine Init Failed"_sv);
        return false;
    }

    pICore_->RegisterAssertHandler(&assertCallback_);

    LinkModule(pICore_, "Benchmarks");

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

void EngineApp::Error(core::string_view errorText)
{
    core::msgbox::show(errorText,
        X_ENGINE_NAME " Start Error"_sv,
        core::msgbox::Style::Error | core::msgbox::Style::Topmost | core::msgbox::Style::DefaultDesktop,
        core::msgbox::Buttons::OK);
}