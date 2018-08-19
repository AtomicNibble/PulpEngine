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

#include <Hashing\crc32.h>
#include <Platform\Window.h>
#include <Platform\Console.h>
#include <Platform\SystemInfo.h>
#include <Platform\Module.h>
#include <Platform\MessageBox.h>
#include <Debugging\InvalidParameterHandler.h>
#include <Debugging\CallStack.h>
#include <Memory\VirtualMem.h>
#include <String\HumanSize.h>
#include <Time\StopWatch.h>

#include <Console.h>

#include "CoreEventDispatcher.h"
#include <Platform\DirectoryWatcher.h>

#include <Threading\JobSystem2.h>

#if X_PLATFORM_WIN32
#include <conio.h>
#endif // !X_PLATFORM_WIN32

namespace
{
#if X_ENABLE_MEMORY_DEBUG_POLICIES

    typedef core::MemoryArena<
        core::GrowingGenericAllocator,
        core::MultiThreadPolicy<core::Spinlock>,
        core::SimpleBoundsChecking,
        core::SimpleMemoryTracking,
        core::SimpleMemoryTagging>
        StrArena;

#else

    typedef core::MemoryArena<
        core::GrowingGenericAllocator,
        core::MultiThreadPolicy<core::Spinlock>,
        core::NoBoundsChecking,
#if X_ENABLE_MEMORY_SIMPLE_TRACKING
        core::SimpleMemoryTracking,
#else
        core::NoMemoryTracking,
#endif // !X_ENABLE_MEMORY_SIMPLE_TRACKING
        core::NoMemoryTagging>
        StrArena;

#endif // !X_ENABLE_MEMORY_DEBUG_POLICIES

    typedef core::MemoryArena<
        StrArena::AllocationPolicy,
        core::SingleThreadPolicy,
        StrArena::BoundsCheckingPolicy,
        StrArena::MemoryTrackingPolicy,
        StrArena::MemoryTaggingPolicy>
        StrArenaST;

} // namespace

CoreGlobals XCore::env_;

XCore::XCore() :
    pWindow_(nullptr),
    pConsole_(nullptr),

    pVsLogger_(nullptr),
    pConsoleLogger_(nullptr),

    pCpuInfo_(nullptr),
    pCrc32_(nullptr),

    moduleDLLHandles_(g_coreArena),
    moduleInterfaces_(g_coreArena),
    converterInterfaces_(g_coreArena),
    assertHandlers_(g_coreArena),

#if X_ENABLE_PROFILER
    pProfiler_(nullptr),
#endif //!X_ENABLE_PROFILER

    pCoreEventDispatcher_(nullptr),

    strAlloc_(1 << 24, core::VirtualMem::GetPageSize() * 2,
        StrArena::getMemoryAlignmentRequirement(8),
        StrArena::getMemoryOffsetRequirement() + 12),

    assetLoader_(g_coreArena, g_coreArena)
{
    X_ASSERT_NOT_NULL(g_coreArena);

    pDirWatcher_ = X_NEW(core::XDirectoryWatcher, g_coreArena, "CoreDirectoryWatcher")(g_coreArena);

    pCoreEventDispatcher_ = X_NEW(core::XCoreEventDispatcher, g_coreArena, "CoreEventDispatch")(vars_, g_coreArena);
    pCoreEventDispatcher_->RegisterListener(this);

    env_.state_ = CoreGlobals::State::STARTING;
    env_.pCore = this;
    env_.pTimer = &time_;
    env_.pDirWatcher = pDirWatcher_;
    //	env_.pMalloc = &g_coreArena;
    env_.pArena = g_coreArena;

    if (initParams_.bThreadSafeStringAlloc) {
        env_.pStrArena = X_NEW(StrArena, g_coreArena, "StrArena")(&strAlloc_, "StrArena");
    }
    else {
        env_.pStrArena = X_NEW(StrArenaST, g_coreArena, "StrArena")(&strAlloc_, "StrArenaST");
    }

    static_assert(StrArena::IS_THREAD_SAFE, "Str arena must be thread safe");
    static_assert(!StrArenaST::IS_THREAD_SAFE, "Single thread StrArean don't need to be thread safe");

    env_.client_ = true;
    env_.dedicated_ = false;

    // created in coreInit.
    //	env_.pJobSys = X_NEW(core::JobSystem, g_coreArena, "JobSystem");
    //env_.pJobSys = X_NEW(core::V2::JobSystem, g_coreArena, "JobSystem");

    if (pDirWatcher_) {
        pDirWatcher_->registerListener(this);
    }
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
    X_DELETE(this, g_coreArena);
}

void XCore::ShutDown()
{
    X_LOG0("Core", "Shutting Down");
    env_.state_ = CoreGlobals::State::CLOSING;

    if (vars_.coreFastShutdown_) {
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
        X_DELETE(pDirWatcher_, g_coreArena);
    }

    // save the vars before we start deleting things.
    if (env_.pConsole && !initParams_.basicConsole()) {
        env_.pConsole->saveChangedVars();
    }

    if (env_.pJobSys) {
        env_.pJobSys->ShutDown();
        core::Mem::DeleteAndNull(env_.pJobSys, g_coreArena);
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

    if (env_.pVideoSys) {
        env_.pVideoSys->shutDown();
        core::SafeRelease(env_.pVideoSys);
    }

    if (pWindow_) {
        pWindow_->Destroy();
        core::Mem::DeleteAndNull(pWindow_, g_coreArena);
    }

    if (pCpuInfo_) {
        core::Mem::DeleteAndNull(pCpuInfo_, g_coreArena);
    }

    if (pCrc32_) {
        core::Mem::DeleteAndNull(pCrc32_, g_coreArena);
    }

#if X_ENABLE_PROFILER
    if (pProfiler_) {
        pProfiler_->shutDown();
        core::Mem::DeleteAndNull(pProfiler_, g_coreArena);
    }
#endif // !X_ENABLE_PROFILER

    if (env_.pFileSys) {
        env_.pFileSys->shutDown();
        core::Mem::DeleteAndNull(env_.pFileSys, g_coreArena);
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
        if (!it->ShutDown()) {
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
        core::Mem::DeleteAndNull(env_.pConsole, g_coreArena);
    }

    if (env_.pStrArena) {
        core::Mem::DeleteAndNull(env_.pStrArena, g_coreArena);
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

        X_DELETE(pVsLogger_, g_coreArena);
        X_DELETE(pConsoleLogger_, g_coreArena);
        X_DELETE(pConsole_, g_coreArena);

        core::Mem::DeleteAndNull(env_.pLog, g_coreArena);
    }

    if (pCoreEventDispatcher_) {
        pCoreEventDispatcher_->RemoveListener(this);
        core::Mem::DeleteAndNull(pCoreEventDispatcher_, g_coreArena);
    }

    for (size_t i = 0; i < moduleDLLHandles_.size(); i++) {
        core::Module::UnLoad(moduleDLLHandles_[i]);
    }

    moduleDLLHandles_.free();

    //	core::invalidParameterHandler::Shutdown();
}

bool XCore::PumpMessages()
{
    if (pWindow_) {
        return pWindow_->PumpMessages() != core::xWindow::Notification::CLOSE;
    }

    // we have no main window
    X_ERROR("PumpMessages", "no main window present");
    return false;
}

void XCore::OnCoreEvent(CoreEvent::Enum event, const CoreEventData& ed)
{
    X_UNUSED(ed);

    switch (event) {
        case CoreEvent::MOVE: {
            core::xWindow::Rect rect = pWindow_->GetRect();
            vars_.updateWinPos(rect.getX1(), rect.getY1());
        } break;
        case CoreEvent::RESIZE: {
            core::xWindow::Rect rect = pWindow_->GetClientRect();
            vars_.updateWinDim(rect.getWidth(), rect.getHeight());
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

    if (action == core::IDirectoryWatcher::Action::MODIFIED) 
    {
        assetLoader_.onFileChanged(pName);
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

    core::LoggerBase::Line Line;
    vsnprintf_s(Line, sizeof(core::LoggerBase::Line), _TRUNCATE, pFormat, args);

    core::msgbox::show(Line,
        X_ENGINE_NAME " Fatal Error",
        core::msgbox::Style::Error | core::msgbox::Style::Topmost | core::msgbox::Style::DefaultDesktop,
        core::msgbox::Buttons::OK);

    X_BREAKPOINT;

    _exit(1);
}

void XCore::WindowPosVarChange(core::ICVar* pVar)
{
    X_UNUSED(pVar);

    int x_pos = vars_.winXPos_;
    int y_pos = vars_.winYPos_;

    core::xWindow* pWin = GetGameWindow();
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
        verbose = core::strUtil::StringToBool(pCmd->GetArg(1));
    }

    ListDisplayDevices(verbose);
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
}

void XCore::ListDisplayDevices(bool verbose) const
{
    core::SysInfo::DisplayDeviceArr devices(g_coreArena);
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