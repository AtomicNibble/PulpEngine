#include "stdafx.h"
#include "Core.h"

#include <IInput.h>
#include <IConsole.h>
#include <ISound.h>
#include <IFileSys.h>
#include <ITimer.h>
#include <I3DEngine.h>
#include <IRender.h>
#include <IScriptSys.h>
#include <IFont.h>
#include <IGame.h>
#include <IPhysics.h>
#include <INetwork.h>
#include <IVideo.h>
#include <IEngineModule.h>
#include <Ilocalisation.h>

#include <Hashing\crc32.h>
#include <Platform\Window.h>
#include <Platform\Console.h>
#include <Platform\SystemInfo.h>
#include <Platform\Module.h>
#include <Platform\MessageBox.h>
#include <Debugging\InvalidParameterHandler.h>
#include <Debugging\CallStack.h>
#include <String\HumanSize.h>
#include <Time\StopWatch.h>

#include <Memory/VirtualMem.h>
#include <Memory/AllocationPolicies/LinearAllocator.h>
#include <Memory/SimpleMemoryArena.h>

#include <Console.h>

#include "SystemTimer.h"
#include "CoreEventDispatcher.h"
#include "ReplaySys.h"
#include <Platform\DirectoryWatcher.h>

#include <Threading\JobSystem2.h>

using namespace core::string_view_literals;

#if X_PLATFORM_WIN32
#include <conio.h>
#endif // !X_PLATFORM_WIN32


namespace
{
    char buf[core::bitUtil::RoundUpToMultiple<size_t>(sizeof(XCore) + 64, 128)];

    core::LinearAllocator linera(buf, buf + sizeof(buf));
    core::SimpleMemoryArena<decltype(linera)> coreInstArena(&linera, "CoreAllocator");

} // namespace


CoreGlobals XCore::env_;

core::NullLog XCore::s_nullLogInst;


XCore* XCore::CreateInstance(void)
{
    return X_NEW_ALIGNED(XCore, &coreInstArena, "XCore", core::Max(X_ALIGN_OF(XCore), 64_sz));
}

XCore::XCore() :
    coreArena_(&coreMalloc_, "CoreAlloc"),
    pWindow_(nullptr),
    pConsole_(nullptr),

    pVsLogger_(nullptr),
    pConsoleLogger_(nullptr),
    pFileLogger_(nullptr),

    pCpuInfo_(nullptr),
    pCrc32_(nullptr),
    pReplaySys_(nullptr),

    numFrames_(0),

    moduleDLLHandles_(&coreArena_),
    moduleInterfaces_(&coreArena_),
    converterInterfaces_(&coreArena_),
    assertHandlers_(&coreArena_),

#if X_ENABLE_PROFILER
    pProfiler_(nullptr),
#endif //!X_ENABLE_PROFILER

    pCoreEventDispatcher_(nullptr),

    strAlloc_(1 << 24, core::VirtualMem::GetPageSize() * 2,
        StrArena::getMemoryAlignmentRequirement(8),
        StrArena::getMemoryOffsetRequirement() + 12),
    args_(&coreArena_)
{

}

XCore::~XCore()
{
    ShutDown();

    env_.pCore = nullptr;
    env_.pTimer = nullptr;
    env_.pDirWatcher = nullptr;
    env_.pArena = nullptr;
}

void XCore::Release()
{
    X_DELETE(this, &coreInstArena);
}

void XCore::ShutDown()
{
    {
        ttZone(gEnv->ctx, "(Core) Shutdown");

        X_LOG0("Core", "Shutting Down");
        env_.state_ = CoreGlobals::State::CLOSING;

        if (vars_.getCoreFastShutdown()) {
            X_LOG0("Core", "Fast shutdown, skipping cleanup");

            // still save modified vars.
            if (env_.pConsole && !initParams_.basicConsole()) {
                env_.pConsole->saveChangedVars();
            }

            moduleInterfaces_.free();
            converterInterfaces_.free();
            return;
        }

        core::StopWatch timer;

        if (pDirWatcher_) {
            pDirWatcher_->shutDown();
            X_DELETE(pDirWatcher_, &coreArena_);
        }

        // save the vars before we start deleting things.
        if (env_.pConsole && !initParams_.basicConsole()) {
            env_.pConsole->saveChangedVars();
        }

        if (env_.pJobSys) {
            env_.pJobSys->ShutDown();
            core::Mem::DeleteAndNull(env_.pJobSys, &coreArena_);
        }

        if (env_.pGame) {
            env_.pGame->shutDown();
            core::SafeRelease(env_.pGame);
        }

        if (env_.pConsole) {
            env_.pConsole->freeRenderResources();
        }

        if (env_.p3DEngine) {
            env_.p3DEngine->shutDown();
            core::SafeRelease(env_.p3DEngine);
        }

        if (env_.pRender) {
            //	env_.pRender->ShutDown();
        }

        if (env_.pPhysics) {
            env_.pPhysics->shutDown();
            core::SafeRelease(env_.pPhysics);
        }

        if (env_.pVideoSys) {
            env_.pVideoSys->shutDown();
            core::SafeRelease(env_.pVideoSys);
        }

        if (env_.pSound) {
            env_.pSound->shutDown();
            core::SafeRelease(env_.pSound);
        }

        if (env_.pFontSys) {
            env_.pFontSys->shutDown();
            core::SafeRelease(env_.pFontSys);
        }

        if (env_.pScriptSys) {
            env_.pScriptSys->shutDown();
            core::SafeRelease(env_.pScriptSys);
        }

        if (env_.pNet) {
            env_.pNet->shutDown();
            core::SafeRelease(env_.pNet);
        }

        if (env_.pLocalisation) {
            core::Mem::DeleteAndNull(env_.pLocalisation, &coreArena_);
        }

        if (pWindow_) {
            pWindow_->Destroy();
            core::Mem::DeleteAndNull(pWindow_, &coreArena_);
        }

        if (pCpuInfo_) {
            core::Mem::DeleteAndNull(pCpuInfo_, &coreArena_);
        }

        if (pCrc32_) {
            core::Mem::DeleteAndNull(pCrc32_, &coreArena_);
        }

        if (pReplaySys_) {
            core::Mem::DeleteAndNull(pReplaySys_, &coreArena_);
        }

    #if X_ENABLE_PROFILER
        if (pProfiler_) {
            pProfiler_->shutDown();
            core::Mem::DeleteAndNull(pProfiler_, &coreArena_);
        }
    #endif // !X_ENABLE_PROFILER

        // clean up file logger, now
        if (pFileLogger_) {

            if (env_.pLog) {
                env_.pLog->RemoveLogger(pFileLogger_);
            }

            X_DELETE(pFileLogger_, &coreArena_);
        }

        if (env_.pFileSys) {
            env_.pFileSys->shutDown();
            core::Mem::DeleteAndNull(env_.pFileSys, &coreArena_);
        }

        // needs to be done after engine, since it has input listners.
        if (env_.pInput) {
            env_.pInput->shutDown();
            core::SafeRelease(env_.pInput);
        }

        if (env_.pRender) {
            env_.pRender->shutDown();

            core::SafeRelease(env_.pRender);
        }

        // shut down interfaces before logging is removed.
        for (auto& it : moduleInterfaces_) {
            auto* pModule = it.pModule.get();
            if (!pModule->ShutDown()) {
                X_ERROR("Core", "error shuting down engine module");
            }
        }
        for (const auto& it : converterInterfaces_) {
            auto* conModule = it.pConModule.get();
            const auto& instance = it.pConverter;
            if (!conModule->ShutDown(instance)) {
                X_ERROR("Core", "error shuting down converter module");
            }
        }

        moduleInterfaces_.free();
        converterInterfaces_.free();

        X_LOG0("Core", "primary shutdown took: 6%gms", timer.GetMilliSeconds());

    #if X_ENABLE_LOGGING
        if (!initParams_.bTesting && initParams_.bPauseShutdown && pConsole_) { // don't pause when testing.
            pConsole_->pressToContinue();
        }
    #endif // !X_ENABLE_LOGGING

        if (env_.pConsole) {
            env_.pConsole->shutDown();
            core::Mem::DeleteAndNull(env_.pConsole, &coreArena_);
        }

        if (env_.pStrArena) {
            core::Mem::DeleteAndNull(env_.pStrArena, &coreArena_);
        }

        // Loggers last.
        if (env_.pLog) {
            //  No need to remove them, if log system is closing.
            //	if (pVsLogger_)
            //		env_.pLog->RemoveLogger(pVsLogger_);
            //	if (pConsoleLogger_)
            //		env_.pLog->RemoveLogger(pConsoleLogger_);

            env_.pLog->ShutDown();

            //if (pConsole)
            //	pConsole->Show(false);

            //	system("PAUSE");

            X_DELETE(pVsLogger_, &coreArena_);
            X_DELETE(pConsoleLogger_, &coreArena_);
            X_DELETE(pConsole_, &coreArena_);

            // if this is not null log.
            if (env_.pLog != &s_nullLogInst) {
                core::Mem::DeleteAndNull(env_.pLog, &coreArena_);
            }
        }

        if (pCoreEventDispatcher_) {
            pCoreEventDispatcher_->RemoveListener(this);
            core::Mem::DeleteAndNull(pCoreEventDispatcher_, &coreArena_);
        }

        for (size_t i = 0; i < moduleDLLHandles_.size(); i++) {
            core::Module::UnLoad(moduleDLLHandles_[i]);
        }

        moduleDLLHandles_.free();
    }

#if TTELEMETRY_ENABLED
    ttShutdownContext(gEnv->ctx);
    ttShutDown();
#endif // TTELEMETRY_ENABLED

    //	core::invalidParameterHandler::Shutdown();
}

void XCore::toggleFullscreen(void)
{
    auto fullscreen = vars_.getFullscreen();

    if (fullscreen > 0)
    {
        pWindow_->SetMode(core::Window::Mode::APPLICATION);

        Recti r;
        r.x1 = vars_.getWinPosX();
        r.y1 = vars_.getWinPosY();
        r.x2 = r.x1 + vars_.getWinWidth();
        r.y2 = r.y1 + vars_.getWinHeight();

        pWindow_->SetRect(r);
        vars_.setFullScreen(0);
    }
    else
    {
        auto rect = pWindow_->GetActiveMonitorRect();

        // work out which device we are on.
        int32_t deviceIdx = 0;
        core::SysInfo::DeviceMode mode;

        for (deviceIdx = 0; ; deviceIdx++)
        {
            if (!core::SysInfo::GetDisplayMode(deviceIdx, mode)) {
                deviceIdx = -1;
                break;
            }

            if (rect.contains(mode.position)) {
                break;
            }
        }

        pWindow_->SetMode(core::Window::Mode::FULLSCREEN);
        pWindow_->SetRect(rect);

        vars_.setMonitorIdx(deviceIdx);
        vars_.setFullScreen(1);
    }
}

bool XCore::PumpMessages(void)
{
    if (pWindow_) {
        return pWindow_->PumpMessages() != core::Window::Notification::CLOSE;
    }

    // we have no main window
    X_ERROR("Core", "PumpMessages: no window present");
    return false;
}

void XCore::OnCoreEvent(const CoreEventData& ed)
{
    switch (ed.event) 
    {
        case CoreEvent::MOVE: 
        case CoreEvent::RESIZE: 
        {
            // i only want to save if not fullscreen
            auto max = pWindow_->isMaximized();
            auto full = pWindow_->getMode() == core::Window::Mode::FULLSCREEN;
            if (!max && !full)
            {
                if (ed.event == CoreEvent::MOVE) {
                    vars_.updateWinPos(ed.move.windowX, ed.move.windowY);
                }
                else {
                    vars_.updateWinDim(ed.resize.width, ed.resize.height);
                }
            }
        } break;
        case CoreEvent::CHANGE_FOCUS:
            if (ed.focus.active != 0) {
                if (pWindow_) {
                    pWindow_->ClipCursorToWindow();
                }
            }
            break;

        default:
            break;
    }
}


bool XCore::OnFileChange(core::IDirectoryWatcher::Action::Enum action,
    const char* pName, const char* pOldName, bool isDirectory)
{
    X_UNUSED(pName, pOldName);

    if (isDirectory) {
        return false;
    }

    if (action == core::IDirectoryWatcher::Action::MODIFIED && assetLoader_)
    {
        assetLoader_->onFileChanged(pName);
    }

    return true;
}

const wchar_t* XCore::GetCommandLineArgForVarW(const wchar_t* pVarName)
{
    X_ASSERT_NOT_NULL(pVarName);

    if (args_.isEmpty()) {
        return nullptr;
    }

    size_t i;
    for (i = 0; i < args_.size(); i++) {
        const CmdArg& arg = args_[i];

        if (arg.getArgc() >= 3) {
            const wchar_t* pCmd = arg.getArgv(0);
            if (core::strUtil::IsEqualCaseInsen(pCmd, L"set")) {
                const wchar_t* pName = arg.getArgv(1);
                if (core::strUtil::IsEqual(pName, pVarName)) {
                    const wchar_t* pValue = arg.getArgv(2);
                    return pValue;
                }
            }
        }
        else if (arg.getArgc() == 2) {
            const wchar_t* pName = arg.getArgv(0);
            if (core::strUtil::IsEqualCaseInsen(pName, pVarName)) {
                const wchar_t* pValue = arg.getArgv(1);
                return pValue;
            }
        }
        else if (arg.getArgc() == 1) {
            const wchar_t* pName = arg.getArgv(0);
            if (core::strUtil::IsEqualCaseInsen(pName, pVarName)) {
                return L"";
            }
        }
    }

    return nullptr;
}

void XCore::OnFatalError(const char* pFormat, va_list args)
{
    core::CallStack::Description Dsc;
    core::CallStack stack(1);

    stack.ToDescription(Dsc);

    X_LOG0("FatalError", "CallStack:\n%s", Dsc);

    core::LoggerBase::Line lineStr;
    int length = vsnprintf_s(lineStr, sizeof(core::LoggerBase::Line), _TRUNCATE, pFormat, args);

    core::msgbox::show(core::string_view(lineStr, length),
        X_ENGINE_NAME " Fatal Error"_sv,
        core::msgbox::Style::Error | core::msgbox::Style::Topmost | core::msgbox::Style::DefaultDesktop,
        core::msgbox::Buttons::OK);

    X_BREAKPOINT;

    _exit(1);
}



void XCore::WindowPosVarChange(core::ICVar* pVar)
{
    X_UNUSED(pVar);

    int x_pos = vars_.getWinPosX();
    int y_pos = vars_.getWinPosY();

    core::Window* pWin = GetGameWindow();
    if (pWin) {
        pWin->MoveTo(x_pos, y_pos);
    }
}

void XCore::WindowSizeVarChange(core::ICVar* pVar)
{
    X_UNUSED(pVar);

}

void XCore::WindowCustomFrameVarChange(core::ICVar* pVar)
{
    bool enabled = (pVar->GetInteger() == 1);

    if (pWindow_) {
        pWindow_->CustomFrame(enabled);
    }
}

void XCore::Cmd_ListProgramArgs(core::IConsoleCmdArgs* pCmd)
{
    X_UNUSED(pCmd);

    ListProgramArgs();
}

void XCore::Cmd_ListDisplayDevices(core::IConsoleCmdArgs* pCmd)
{
    bool verbose = false;

    if (pCmd->GetArgCount() > 1) {
        auto verboseStr = pCmd->GetArg(1);
        verbose = core::strUtil::StringToBool(verboseStr.begin(), verboseStr.end());
    }

    ListDisplayDevices(verbose);
}

void XCore::Cmd_ReplayRecord(core::IConsoleCmdArgs* pCmd)
{
    core::string_view name;

    if (pCmd->GetArgCount() > 1) {
        name = pCmd->GetArg(1);
    }
    else {
        name = "replay"_sv;
    }

    pReplaySys_->record(name);
}

void XCore::Cmd_ReplayPlay(core::IConsoleCmdArgs* pCmd)
{
    core::string_view name;

    if (pCmd->GetArgCount() > 1) {
        name = pCmd->GetArg(1);
    }
    else {
        name = "replay"_sv;
    }

    pReplaySys_->play(name);
}

void XCore::Cmd_ReplayStop(core::IConsoleCmdArgs* pCmd)
{
    X_UNUSED(pCmd);

    pReplaySys_->stop();
}

void XCore::ListProgramArgs(void)
{
    size_t i, num = args_.size();

    X_LOG0("AppArgs", "----------- ^8Program Args(%" PRIuS ")^7 ----------", num);

    core::StackString<1024 + 128> merged;

    for (i = 0; i < num; i++) {
        const auto& arg = args_[i];
        size_t j, argsNum = arg.getArgc();

        merged.appendFmt("(%" PRIuS ") ", i);

        for (j = 0; j < argsNum; j++) {
            merged.appendFmt("^2%ls^7", arg.getArgv(j));
            if ((j + 1) < argsNum) {
                merged.append(" -> ");
            }
        }

        X_LOG0("AppArgs", "%s", merged.c_str());
    }

    X_LOG0("AppArgs", "---------- ^8Program Args End^7 ----------");
}

void XCore::LogSystemInfo(void) const
{
    core::SysInfo::UserNameStr userName;
    core::SysInfo::LanguageStr lang;
    core::SysInfo::MemInfo memInfo;
    core::SysInfo::DeviceMode mode;

    core::SysInfo::GetUserName(userName);
    core::SysInfo::GetLanguage(lang);
    core::SysInfo::GetSystemMemInfo(memInfo);
    core::SysInfo::GetCurrentDisplayMode(mode);

    core::HumanSize::Str s1, s2, s3;

    X_LOG0("SysInfo", "UserName: \"%ls\"", userName);
    X_LOG0("SysInfo", "Language: \"%ls\"", lang);
    X_LOG0("SysInfo", "PhysicalMem ^6%s^7 available ^6%s^7 virtual ^6%s^7 used ^6%" PRIu32 "%%",
        core::HumanSize::toString(s1, memInfo.TotalPhys),
        core::HumanSize::toString(s2, memInfo.AvailPhys),
        core::HumanSize::toString(s3, memInfo.TotalVirtual),
        memInfo.dwMemoryLoad);
    X_LOG0("SysInfo", "Display: ^6%d^7x^6%d^7x^6%d",
        mode.pelsWidth,
        mode.pelsHeight,
        mode.bitsPerPel);

    X_LOG0("Core", "TimerFreq: %" PRIi64, core::SysTimer::GetTickPerSec());
}

void XCore::ListDisplayDevices(bool verbose)
{
    core::SysInfo::DisplayDeviceArr devices(&coreArena_);
    core::SysInfo::GetDisplayDevices(devices);

    X_LOG0("SysInfo", "DisplayDevices: %" PRIuS, devices.size());

    for (auto& device : devices)
    {
        X_LOG0("SysInfo", "DeviceName: \"%ls\"", device.deviceName.c_str());
        X_LOG0("SysInfo", "DeviceString: \"%ls\"", device.deviceString.c_str());
        X_LOG0("SysInfo", "DeviceID: \"%ls\"", device.deviceID.c_str());
        X_LOG0("SysInfo", "DeviceKey: \"%ls\"", device.deviceKey.c_str());
        X_LOG0("SysInfo", "Monitors: %" PRIuS, device.monitors.size());

        if (device.monitors.isEmpty()) {
            continue;
        }

        X_LOG_BULLET;
        for (auto& monitor : device.monitors)
        {
            X_LOG0("SysInfo", "DeviceName: \"%ls\"", monitor.deviceName.c_str());
            X_LOG0("SysInfo", "DeviceString: \"%ls\"", monitor.deviceString.c_str());
            X_LOG0("SysInfo", "DeviceID: \"%ls\"", monitor.deviceID.c_str());
            X_LOG0("SysInfo", "DeviceKey: \"%ls\"", monitor.deviceKey.c_str());

            auto printMode = [](const core::SysInfo::DeviceMode& mode) {
                X_LOG0("SysInfo", "PosX: %" PRIu32, mode.position.x);
                X_LOG0("SysInfo", "PosY: %" PRIu32, mode.position.y);
                X_LOG0("SysInfo", "Width: %" PRIu32, mode.pelsWidth);
                X_LOG0("SysInfo", "Height: %" PRIu32, mode.pelsHeight);
                X_LOG0("SysInfo", "BitsPerPel: %" PRIu32, mode.bitsPerPel);
                X_LOG0("SysInfo", "DispalyFrequency: %" PRIu32, mode.dispalyFrequency);
            };
            
            X_LOG0("SysInfo", "CurrentMode:");
            X_LOG_BULLET;
            printMode(monitor.currentMode);

            if (!verbose) {
                continue;
            }

            for (auto& mode : monitor.modes)
            {
                X_LOG0("SysInfo", "------------------------");
                printMode(mode);
            }
        }

        X_LOG0("SysInfo", "");
    }
}

// -------------------------------

core::IDirectoryWatcher* XCore::GetDirWatcher(void)
{
    return pDirWatcher_;
}

ICoreEventDispatcher* XCore::GetCoreEventDispatcher(void)
{
    return pCoreEventDispatcher_;
}

X_NAMESPACE_BEGIN(core)

X_NAMESPACE_END
