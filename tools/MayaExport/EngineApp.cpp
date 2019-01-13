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

X_LINK_ENGINE_LIB("Core");

using namespace core::string_view_literals;

EngineApp::EngineApp() :
    pICore_(nullptr),
    hSystemHandle_(core::Module::NULL_HANDLE)
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

    CoreInitParams params;
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
//    params.pConsoleWnd = nullptr;
    params.pCoreArena = g_arena;
    params.bLoadSymbols = false;
    params.bFileSysWorkingDir = true;

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

void EngineApp::Error(core::string_view errorText)
{
    core::msgbox::show(errorText,
        X_ENGINE_NAME " Start Error"_sv,
        core::msgbox::Style::Error | core::msgbox::Style::Topmost | core::msgbox::Style::DefaultDesktop,
        core::msgbox::Buttons::OK);
}