#include "stdafx.h"
#include "Console.h"
#include "Core.h"
#include "Log.h"
#include "Localisation/Localisation.h"

#include <String\CmdArgs.h>
#include <String\HumanSize.h>

#include "NullImplementation\ConsoleNULL.h"
#include "NullImplementation\NullInput.h"

#include <Debugging\InvalidParameterHandler.h>
#include <Debugging\SymbolResolution.h>
#include <Debugging\ExceptionHandler.h>

#include <Random\MultiplyWithCarry.h>
#include <Random\XorShift.h>

#include <Hashing\crc32.h>
#include <Hashing\sha1.h>

#include <Threading\JobSystem2.h>
#include <Time\StopWatch.h>

#include <IInput.h>
#include <IEngineModule.h>
#include <IScriptSys.h>
#include <IRender.h>
#include <I3DEngine.h>
#include <IFont.h>
#include <IFileSys.h>
#include <IGame.h>
#include <IJobSystem.h>
#include <INetwork.h>
#include <ISound.h>
#include <IPhysics.h>
#include <IVideo.h>

#include "Logging\Logger.h"
#include "Logging\FilterPolicies\LoggerNoFilterPolicy.h"
#include "Logging\FilterPolicies\LoggerVerbosityFilterPolicy.h"
#include "Logging\FormatPolicies\LoggerExtendedFormatPolicy.h"
#include "Logging\FormatPolicies\LoggerSimpleFormatPolicy.h"
#include "Logging\FormatPolicies\LoggerFullFormatPolicy.h"
#include "Logging\WritePolicies\LoggerDebuggerWritePolicy.h"
#include "Logging\WritePolicies\LoggerConsoleWritePolicy.h"
#include "Logging\WritePolicies\LoggerFileWritePolicy.h"

#include <Extension\IEngineUnknown.h>
#include <Extension\IEngineFactory.h>
#include <Extension\EngineCreateClass.h>

#include <Memory\MemInfo.h>
#include <Platform\MessageBox.h>
#include <Platform\DirectoryWatcher.h>
#include <Platform\SystemInfo.h>

#include "PotatoFactoryRegistryImpl.h"

// Logging
#include "Platform\Console.h"
#include "Platform\Window.h"
#include "FileSys\xFileSys.h"

#include "SystemTimer.h"

using namespace core::string_view_literals;

#if defined(WIN32)

#include <VersionHelpers.h>

#endif // !defined(WIN32)

X_USING_NAMESPACE;

#define DLL_INPUT X_ENGINE_OUTPUT_PREFIX "Input"
#define DLL_FONT X_ENGINE_OUTPUT_PREFIX "Font"
#define DLL_SOUND X_ENGINE_OUTPUT_PREFIX "Sound"
#define DLL_SCRIPT X_ENGINE_OUTPUT_PREFIX "Script"
#define DLL_RENDER X_ENGINE_OUTPUT_PREFIX "RenderDx12"
#define DLL_RENDER_NULL X_ENGINE_OUTPUT_PREFIX "RenderNull"
#define DLL_3D_ENGINE X_ENGINE_OUTPUT_PREFIX "3DEngine"
#define DLL_GAME_DLL X_ENGINE_OUTPUT_PREFIX "GameDLL"
#define DLL_PHYSICS X_ENGINE_OUTPUT_PREFIX "Physics"
#define DLL_NET X_ENGINE_OUTPUT_PREFIX "Network"
#define DLL_VIDEO X_ENGINE_OUTPUT_PREFIX "Video"

#define DLL_MODULE_INIT_ICORE "LinkModule"
#define DLL_INITFUNC_INPUT "CreateInput"

namespace
{
    typedef core::traits::Function<void*(ICore*, const char*)> ModuleLinkfunc;


    typedef core::Logger<
        core::LoggerNoFilterPolicy,
        core::LoggerFullFormatPolicy,
        core::LoggerDebuggerWritePolicy>
        VisualStudioLogger;

    typedef core::Logger<
        core::LoggerVerbosityFilterPolicy,
        core::LoggerSimpleFormatPolicy,
        core::LoggerConsoleWritePolicy>
        ConsoleLogger;

    typedef core::Logger<
        core::LoggerNoFilterPolicy,
        core::LoggerSimpleFormatPolicy,
        core::LoggerFileWritePolicy>
        FileLogger;

} // namespace

//////////////////////////////////////////////////////////////////////////
#if !defined(X_LIB)

core::Module::Handle XCore::LoadDLL(const char* pDllName)
{
    X_PROFILE_NO_HISTORY_BEGIN("LoadDll", core::profiler::SubSys::CORE);

    core::Module::Handle handle = core::Module::Load(pDllName);
    if (!handle) {
        return nullptr;
    }

    ModuleLinkfunc::Pointer pfnModuleInitICore = reinterpret_cast<ModuleLinkfunc::Pointer>(
        core::Module::GetProc(handle, DLL_MODULE_INIT_ICORE));

    if (pfnModuleInitICore) {
        pfnModuleInitICore(this, pDllName);
    }

    return handle;
}
#endif //#if !defined(X_LIB)

bool XCore::IntializeLoadedEngineModule(const char* pDllName, const char* pModuleClassName)
{
    X_PROFILE_NO_HISTORY_BEGIN("EngModuleInit", core::profiler::SubSys::CORE);

    for (auto& c : moduleInterfaces_) {
        if (c.dllName.compareCaseInsen(pDllName) && c.moduleClassName.compareCaseInsen(pModuleClassName)) {
            c.addReference();
            return true;
        }
    }

#if !defined(X_LIB)
    core::Module::Handle handle = core::Module::Load(pDllName);

    ModuleLinkfunc::Pointer pfnModuleInitICore = reinterpret_cast<ModuleLinkfunc::Pointer>(
        core::Module::GetProc(handle, DLL_MODULE_INIT_ICORE));

    if (pfnModuleInitICore) {
        pfnModuleInitICore(this, pDllName);
    }

#endif

    std::shared_ptr<IEngineModule> pModule;
    if (EngineCreateClassInstance(pModuleClassName, pModule)) {
        if (!pModule->Initialize(env_, initParams_)) {
            X_ERROR("Core", "failed to initialize IEng: %s -> %s", pDllName, pModuleClassName);
            return false;
        }
    }
    else {
        X_ERROR("Core", "failed to find interface: %s -> %s", pDllName, pModuleClassName);
        return false;
    }

    // auto clean up by core shutdown.
    EngineModule module;
    module.dllName = pDllName;
    module.moduleClassName = pModuleClassName;
    module.pModule = pModule;

    moduleInterfaces_.emplace_back(std::move(module));

    return true;
}

bool XCore::IntializeLoadedConverterModule(const char* pDllName, const char* pModuleClassName,
    IConverterModule** pConvertModuleOut, IConverter** pConverterInstance)
{
    X_PROFILE_NO_HISTORY_BEGIN("ConModuleInit", core::profiler::SubSys::CORE);

    for (auto& c : converterInterfaces_) {
        if (c.dllName.compareCaseInsen(pDllName) && c.moduleClassName.compareCaseInsen(pModuleClassName)) {
            c.addReference();

            if (pConvertModuleOut) {
                *pConvertModuleOut = c.pConModule.get();
            }

            if (pConverterInstance) {
                *pConverterInstance = c.pConverter;
            }

            return true;
        }
    }

#if !defined(X_LIB)
    core::Module::Handle handle = core::Module::Load(pDllName);

    ModuleLinkfunc::Pointer pfnModuleInitICore = reinterpret_cast<ModuleLinkfunc::Pointer>(
        core::Module::GetProc(handle, DLL_MODULE_INIT_ICORE));

    if (pfnModuleInitICore) {
        pfnModuleInitICore(this, pDllName);
    }

#endif

    std::shared_ptr<IConverterModule> pModule;
    IConverter* pConverter = nullptr;

    if (EngineCreateClassInstance(pModuleClassName, pModule)) {
        pConverter = pModule->Initialize();
        if (!pConverter) {
            X_ERROR("Core", "failed to initialize ICon: %s -> %s", pDllName, pModuleClassName);
            return false;
        }
    }
    else {
        X_ERROR("Core", "failed to find interface: %s -> %s", pDllName, pModuleClassName);
        return false;
    }

    // this is a little more messy than i'd like in terms of how the ref counting is done.
    // ideally we need to remove the responsibility of creating a convter instance on to the caller
    // that way creating multiple converter instances is supported (no use case ofr it yet but supported by all converter modules)

    ConverterModule conMod;
    conMod.dllName = pDllName;
    conMod.moduleClassName = pModuleClassName;
    conMod.pConverter = pConverter;
    conMod.pConModule = pModule;

    converterInterfaces_.emplace_back(conMod);

    if (pConvertModuleOut) {
        *pConvertModuleOut = pModule.get();
    }

    if (pConverterInstance) {
        *pConverterInstance = pConverter;
    }

    return true;
}

bool XCore::FreeConverterModule(IConverterModule* pConvertModule)
{
    for (size_t i = 0; i < converterInterfaces_.size(); i++) {
        auto& c = converterInterfaces_[i];
        if (c.pConModule.get() == pConvertModule) {
            if (c.removeReference() == 0) {
                if (!c.pConModule->ShutDown(c.pConverter)) {
                    X_ERROR("Core", "error shuting down converter module");
                }

                converterInterfaces_.removeIndex(i);
            }

            return true;
        }
    }
    return false;
}

bool XCore::IntializeEngineModule(const char* pDllName, const char* pModuleClassName, const CoreInitParams& initParams)
{
    X_PROFILE_NO_HISTORY_BEGIN("ModuleInit", core::profiler::SubSys::CORE);

    core::Path<char> path(pDllName);
    path.setExtension(".dll");

    core::StopWatch time;

#if !defined(X_LIB)
    core::XProcessMemInfo memStart, memEnd;
    core::GetProcessMemInfo(memStart);
#endif // #if !defined(X_LIB)

#if !defined(X_LIB)
    auto hModule = LoadDLL(path.c_str());
    if (!hModule) {
        if (gEnv && gEnv->pLog) {
            X_ERROR("Core", "Failed to load engine module: %s", path.c_str());
        }
        return false;
    }

    moduleDLLHandles_.push_back(hModule);
#endif // #if !defined(X_LIB)

    bool res = false;

    std::shared_ptr<IEngineModule> pModule;
    if (EngineCreateClassInstance(pModuleClassName, pModule)) {
        res = pModule->Initialize(env_, initParams);
    }
    else {
        X_ERROR("Core", "failed to find interface: %s -> %s", pDllName, pModuleClassName);
        return false;
    }

    EngineModule module;
    module.dllName = pDllName;
    module.moduleClassName = pModuleClassName;
    module.pModule = pModule;

    moduleInterfaces_.emplace_back(std::move(module));

    const char* pName = core::strUtil::Find(pDllName, pDllName + core::strUtil::strlen(pDllName), "_");
    X_UNUSED(pName);

#if !defined(X_LIB)
    core::GetProcessMemInfo(memEnd);
    int64 memUsed = static_cast<int64_t>(memEnd.WorkingSetSize) - static_cast<int64_t>(memStart.WorkingSetSize);
    core::HumanSize::Str sizeStr;
    X_LOG0("Engine", "ModuleInit \"%s\", MemUsage=%s ^6%gms", pName ? (pName + 1) : pDllName, core::HumanSize::toString(sizeStr, memUsed), time.GetMilliSeconds());
#else

#if X_SUPER == 0
    X_LOG0("Engine", "ModuleInit \"%s\": %s ^6%gms", pName ? (pName + 1) : pDllName, res ? "OK" : "Fail", time.GetMilliSeconds());
#endif // !X_SUPER

#endif // #if !defined(X_LIB)

    return res;
}

//////////////////////////////////////////////////////////////////////////
bool XCore::Init(const CoreInitParams& startupParams)
{
    core::StopWatch time;

    // init the system baby!
    gEnv->mainThreadId = core::Thread::getCurrentID();
    gEnv->seed == startupParams.seed;
    gEnv->isGame_ = startupParams.bIsGame;

    core::invalidParameterHandler::Startup();
    core::exceptionHandler::Startup();

    if (startupParams.bProfileSysEnabled) {
#if X_ENABLE_PROFILER
        pProfiler_ = X_NEW(core::profiler::XProfileSys, g_coreArena, "ProfileSys")(g_coreArena);

        env_.pProfiler = pProfiler_;
#endif // !X_ENABLE_PROFILER
    }

    X_PROFILE_NO_HISTORY_BEGIN("CoreInit", core::profiler::SubSys::CORE);

    if (startupParams.loadSymbols()) {
        X_PROFILE_NO_HISTORY_BEGIN("SymResInit", core::profiler::SubSys::CORE);

        core::symbolResolution::Startup();
    }
    initParams_ = startupParams;

#if defined(WIN32)
    {
        bool bIsWindowsXPorLater = ::IsWindowsXPOrGreater();

        if (!bIsWindowsXPorLater) {
            core::msgbox::show("Versions of windows older than and including XP are not supported."_sv,
                X_ENGINE_NAME " Start Error"_sv,
                core::msgbox::Style::Error | core::msgbox::Style::Topmost | core::msgbox::Style::DefaultDesktop,
                core::msgbox::Buttons::OK);
            return false;
        }
    }
#endif

    if (!ParseCmdArgs(startupParams.pCmdLine)) {
        return false;
    }


    // #------------------------- Logging -----------------------
    if (!InitLogging(startupParams)) {
        return false;
    }


    // #------------------------- SysTimerM ------------------------
    core::SysTimer::Startup();
    env_.timerFreq = core::SysTimer::GetTickPerSec();

    // #------------------------- PSRNG ------------------------
    if (!parseSeed(startupParams.seed)) {
        return false;
    }

    // #------------------------- JOB SYSTEM ------------------------
    if (startupParams.jobSystemEnabled()) {
        env_.pJobSys = X_NEW(core::V2::JobSystem, g_coreArena, "JobSystem")(g_coreArena);
    }

    // #------------------------- FileSystem --------------------
    if (!InitFileSys(startupParams)) {
        return false;
    }

    // #------------------------- File Logging -----------------------
    if (!InitFileLogging(startupParams)) {
        return false;
    }

    // #------------------------- Create Console ----------------
    if (!startupParams.isCoreOnly() || startupParams.basicConsole()) {
        env_.pConsole = X_NEW(core::XConsole, g_coreArena, "ConsoleSys");
        // register the commands so they can be used before Console::Init
        env_.pConsole->registerVars();
        env_.pConsole->registerCmds();

        // register commands
        registerCmds();
    }
    else {
        // use a null console saves me adding a branch
        // for every var register command.
        // and i'm still able to detect var registers before core is init. :)
        env_.pConsole = X_NEW(core::XConsoleNULL, g_coreArena, "NullConsole");
    }

    // we always register the vars even if console null.
    registerVars(startupParams);

    if (!env_.pConsole->init(this, startupParams.basicConsole())) {
        return false;
    }

#if X_ENABLE_PROFILER
    // reg profiler vars.
    if (pProfiler_) {
        pProfiler_->registerVars();
        pProfiler_->registerCmds();
    }
#endif // !X_ENABLE_PROFILER

    // register verbosity vars.
    if (pConsoleLogger_) {
        auto* pConsoleLogger = static_cast<ConsoleLogger*>(pConsoleLogger_);
        pConsoleLogger->GetFilterPolicy().RegisterVars();
    }

    if (pDirWatcher_) {
        pDirWatcher_->init();
    }

    // register filesystemvars.
    env_.pFileSys->registerVars();
    env_.pFileSys->registerCmds();

    // regeister window vars.
    core::Window::RegisterVars();

    // Load the default config.
    if (!startupParams.isCoreOnly()) {
        X_PROFILE_NO_HISTORY_BEGIN("ConfigLoad", core::profiler::SubSys::CORE);

        if (!env_.pConsole->loadAndExecConfigFile("default.cfg"_sv)) {
            return false;
        }

        if (!env_.pConsole->loadAndExecConfigFile(core::string_view(core::XConsole::USER_CFG_FILE_NAME))) {
            // this is not required.
        }
    }

    // #------------------------- CPU Info ----------------------
    pCpuInfo_ = X_NEW(core::CpuInfo, g_coreArena, "CpuInfo");
    // #------------------------- Crc32 ----------------------
    pCrc32_ = X_NEW(core::Crc32, g_coreArena, "Crc32");

    // Call init on objects from before so they can register vars.
    env_.pLog->Init();

    if (!startupParams.isCoreOnly() && !startupParams.basicConsole()) {
        LogSystemInfo();
    }

    // #------------------------- TIMER ------------------------
    if (!time_.init(this)) {
        return false;
    }
    
    // #------------------------- JOB SYSTEM ------------------------
    if (env_.pJobSys) {
        env_.pJobSys->StartUp(vars_.getSchedulerNumThreads());
    }

    // #------------------------- FileSystem Workers ------------------------
    if (startupParams.jobSystemEnabled()) {
        env_.pFileSys->initWorker();
    }

    if (!startupParams.isCoreOnly()) {

        // #------------------------- Locale ------------------------
        {
            auto loc = core::makeUnique<locale::Localisation>(g_coreArena, g_coreArena);

            // TODO: support other langs :D
            core::Path<char> path("strings/english/str.json");

            if (!loc->loadDict(path)) {
                X_ERROR("Core", "Failed to load string dict");
                return false;
            }

            env_.pLocalisation = loc.release();
        }

        // #------------------------- Input ------------------------
        if (!InitInput(startupParams)) {
            return false;
        }

        // #------------------------- Font -------------------------
        if (!InitFont(startupParams)) {
            return false;
        }

        // #------------------------- ScriptSys -------------------------
        if (!InitScriptSys(startupParams)) {
            return false;
        }

        // #------------------------- Sound -------------------------
        if (!InitSound(startupParams)) {
            return false;
        }

        // #------------------------- Physics -------------------------
        if (!InitPhysics(startupParams)) {
            return false;
        }
    }
    else {
        // optional enable script system in core only mode.
        if (startupParams.ScriptSystem()) {
            if (!InitScriptSys(startupParams)) {
                return false;
            }
        }
    }

#if X_ENABLE_PROFILER
    if (pProfiler_) {
        if (!pProfiler_->init(this)) {
            return false;
        }
    }
#endif // !X_ENABLE_PROFILER

    if (startupParams.bEnableNetowrking) {
        // #------------------------- Networking -------------------------
        if (!InitNet(startupParams)) {
            return false;
        }
    }

    if (startupParams.bEnableVideo) {
        // #------------------------- Video -------------------------
        if (!InitVideo(startupParams)) {
            return false;
        }
    }

    if (!startupParams.bTesting && !startupParams.isCoreOnly()) {
        // create a window
        pWindow_ = X_NEW(core::Window, g_coreArena, "GameWindow");

        wchar_t titleW[128];
        const char* pTitle = X_ENGINE_NAME " Engine " X_CPUSTRING
#if X_SUPER == 0
                                           " (fps:0, 0ms) Time: 0(x1)"
#endif
            ;

        auto mode = core::Window::Mode::APPLICATION;
        Recti r;

        if (vars_.getFullscreen()) {
            mode = core::Window::Mode::FULLSCREEN;
            
            auto monitor = vars_.getMonitor();
            
            core::SysInfo::DeviceMode dm;
            if (monitor >= 0 && core::SysInfo::GetDisplayMode(monitor, dm))
            {
                r.set(
                    dm.position.x,
                    dm.position.y,
                    dm.position.x + dm.pelsWidth,
                    dm.position.y + dm.pelsHeight
                );
            }
            else
            {
                r = core::Window::GetPrimaryRect();
            }

        }
        else {
            r.x1 = vars_.getWinPosX();
            r.y1 = vars_.getWinPosY();
            r.x2 = r.x1 + vars_.getWinWidth();
            r.y2 = r.y1 + vars_.getWinHeight();
        }

        if (!pWindow_->Create(core::strUtil::Convert(pTitle, titleW), r, mode)) {
            return false;
        }

        //	pWindow_->CustomFrame(true);
        pWindow_->HideClientCursor(true);

        // Alogn it
        if (pConsole_) {
        //    pWindow_->AlignTo(core::Window::GetPrimaryRect(),
        //        Alignment::TOP_ALIGN | Alignment::RIGHT_ALIGN);
        }
    }

    // #------------------------- Renderer -------------------------
    if (!startupParams.isCoreOnly() || startupParams.bTesting) {
        if (!InitRenderSys(startupParams)) {
            return false;
        }
    }

    // sync net inits before 3d engine.
    if (env_.pNet) {
        if (!env_.pNet->asyncInitFinalize()) {
            return false;
        }
    }

    //  #------------------------- 3DEngine -------------------------
    if (!startupParams.bTesting && !startupParams.isCoreOnly()) {
        if (!Init3DEngine(startupParams)) {
            return false;
        }

#if X_ENABLE_PROFILER
        if (pProfiler_) {
            if (!pProfiler_->loadRenderResources()) {
                return false;
            }
        }
#endif // !X_ENABLE_PROFILER
    }

    //  #------------------------- Game Dll -------------------------
    if (!startupParams.bTesting && !startupParams.isCoreOnly()) {
        if (!InitGameDll(startupParams)) {
            return false;
        }
    }

    // #------------------------- Console ---------------------------
    if (!InitConsole(startupParams)) {
        return false;
    }

    if (startupParams.loadSymbols()) {
        X_PROFILE_NO_HISTORY_BEGIN("SymResRefresh", core::profiler::SubSys::CORE);
        core::symbolResolution::Refresh();
    }

    // #------------------------ Async Init --------------------------
    {
        // Sync up any asynchronous initialization so we are fully init.
        // This is stuff like loading default assets, that can be done in background to
        // minamize startup time.
        if (!InitAsyncWait()) {
            return false;
        }
    }

    // show the window
    if (pWindow_) {
        pWindow_->Show();
        //	No longer needed since I call this on window activate events.
        //	pWindow_->ClipCursorToWindow();
    }

    X_LOG1("Core", "Core startup: ^6%gms", time.GetMilliSeconds());

    return true;
}

bool XCore::InitAsyncWait(void)
{
    X_PROFILE_NO_HISTORY_BEGIN("AsyncInitFin", core::profiler::SubSys::CORE);

    X_LOG1("Core", "AsynInit wait");

    // we should call all even if one fails.
    // aka we must wait for all to finish even if some fail.
    bool allOk = true;

    if (env_.pConsole) {
        allOk &= env_.pConsole->asyncInitFinalize();
    }

    // wait for default font to fully load.
    if (env_.pFontSys) {
        allOk &= env_.pFontSys->asyncInitFinalize();
    }

    if (env_.p3DEngine) {
        allOk &= env_.p3DEngine->asyncInitFinalize();
    }

#if X_ENABLE_PROFILER
    if (pProfiler_ && !pProfiler_->asyncInitFinalize()) {
        return false;
    }
#endif // !X_ENABLE_PROFILER

    if (env_.pSound) {
        allOk &= env_.pSound->asyncInitFinalize();
    }

    if (env_.pGame) {
        allOk &= env_.pGame->asyncInitFinalize();
    }

    if (env_.pScriptSys) {
        allOk &= env_.pScriptSys->asyncInitFinalize();
    }

    return allOk;
}

bool XCore::ParseCmdArgs(const wchar_t* pArgs)
{
    // should never be null
    if (!pArgs) {
        return false;
    }

    // tokenize all the args
    core::CmdArgs<4096, wchar_t> args(pArgs);

    // now group them based on '+'
    size_t i;
    size_t num = args.getArgc();

    for (i = 0; i < num; i++) {
        if (args_.size() >= args_.capacity()) {
            X_WARNING("Core", "Ignoring %" PRIuS " remaning command args, max reached: %" PRIuS, num - args_.size(), args_.capacity());
            break;
        }

        const wchar_t* pArg = args.getArgv(i);
        if ((*pArg == L'+' || *pArg == L'-') && core::strUtil::strlen(pArg) > 1) {
            args_.AddOne().appendArg(args.getArgv(i) + 1);
        }
        else {
            if (args_.isEmpty()) {
                args_.AddOne();
            }
            args_[args_.size() - 1].appendArg(args.getArgv(i));
        }
    }

    if (GetCommandLineArgForVarW(L"nopause")) {
        env_.noPause_ = true;
    }

    return true;
}

bool XCore::parseSeed(Vec4i seed)
{
    if (seed == Vec4i::zero()) {
        core::XProcessMemInfo meminfo;
        core::GetProcessMemInfo(meminfo);

        core::Hash::SHA1 sha1;
        sha1.update(core::Thread::getCurrentID());
        sha1.update(core::SysTimer::Get());
        sha1.update(meminfo);

        auto digest = sha1.finalize();
        std::memcpy(&seed, digest.bytes, core::Min(sizeof(seed), sizeof(digest)));
    }

    env_.xorShift.setSeed(seed);

    return true;
}

bool XCore::InitFileSys(const CoreInitParams& initParams)
{
    X_UNUSED(initParams);
    env_.pFileSys = X_NEW_ALIGNED(core::xFileSys, g_coreArena, "FileSys", 8)(g_coreArena);

    if (env_.pFileSys) {
        if (!env_.pFileSys->init(initParams)) {
            X_ERROR("Core", "Failed to init filesystem");
            return false;
        }
    }

    return env_.pFileSys != nullptr;
}

//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
bool XCore::InitLogging(const CoreInitParams& initParams)
{
    env_.pLog = X_NEW_ALIGNED(core::XLog, g_coreArena, "LogSystem", 8);

#if X_ENABLE_LOGGING
    if (env_.pLog) {
        if (initParams.bVsLog) {
            pVsLogger_ = X_NEW(VisualStudioLogger, g_coreArena, "VSLogger");
            env_.pLog->AddLogger(pVsLogger_);
        }
        if (initParams.bConsoleLog) {

            const auto& desc = initParams.consoleDesc;
            
            pConsole_ = X_NEW(core::Console, g_coreArena, "ExternalConsoleLog")(core::string_view(desc.pTitle));
            pConsole_->setSize(desc.windowWidth, desc.windowHeight, desc.numLines);
            pConsole_->moveTo(desc.x, desc.y);
            
            env_.pConsoleWnd = pConsole_;

            pConsoleLogger_ = X_NEW(ConsoleLogger, g_coreArena, "ConsoleLogger")(
                ConsoleLogger::FilterPolicy(2, "console"),
                ConsoleLogger::FormatPolicy(),
                ConsoleLogger::WritePolicy(*pConsole_));

            env_.pLog->AddLogger(pConsoleLogger_);
        }
    }

#else
    X_UNUSED(initParams);
#endif
    return env_.pLog != nullptr;
}

bool XCore::InitFileLogging(const CoreInitParams& initParams)
{
#if X_ENABLE_LOGGING
    if (env_.pLog && initParams.bFileLog) {

        core::Path<> path("log.txt");

        pFileLogger_ = X_NEW(FileLogger, g_coreArena, "FileLogger")(
            FileLogger::FilterPolicy(),
            FileLogger::FormatPolicy(),
            FileLogger::WritePolicy(env_.pFileSys, path));

        env_.pLog->AddLogger(pFileLogger_);
    }

#else
    X_UNUSED(initParams);
#endif

    return true;
}

bool XCore::InitConsole(const CoreInitParams& initParams)
{
    // console can load it's render resources now.
    if (!initParams.basicConsole()) {
        if (!env_.pConsole->loadRenderResources()) {
            return false;
        }
    }

    return true;
}

bool XCore::InitInput(const CoreInitParams& initParams)
{
    if (initParams.bSkipInput) {
        env_.pInput = X_NEW(input::XNullInput, g_coreArena, "NullInput");
        return true;
    }

    if (!IntializeEngineModule(DLL_INPUT, "Engine_Input", initParams)) {
        return false;
    }

    X_ASSERT_NOT_NULL(env_.pInput);

    env_.pInput->registerVars();
    env_.pInput->registerCmds();

    if (!env_.pInput->init()) {
        X_ERROR("Font", "failed to init input system");
        return false;
    }

    return true;
}

bool XCore::InitFont(const CoreInitParams& initParams)
{
    if (!IntializeEngineModule(DLL_FONT, "Engine_Font", initParams)) {
        return false;
    }

    X_ASSERT_NOT_NULL(env_.pFontSys);

    env_.pFontSys->registerVars();
    env_.pFontSys->registerCmds();

    if (!env_.pFontSys->init()) {
        X_ERROR("Font", "failed to init font system");
        return false;
    }

    return true;
}

bool XCore::InitSound(const CoreInitParams& initParams)
{
    if (initParams.bSkipSound) {
        env_.pSound = nullptr;
        return true;
    }

    if (!IntializeEngineModule(DLL_SOUND, "Engine_Sound", initParams)) {
        return false;
    }

    X_ASSERT_NOT_NULL(env_.pSound);

    env_.pSound->registerVars();
    env_.pSound->registerCmds();

    if (!env_.pSound->init()) {
        X_ERROR("Font", "failed to init sound system");
        return false;
    }

    return true;
}

bool XCore::InitPhysics(const CoreInitParams& initParams)
{
    if (!IntializeEngineModule(DLL_PHYSICS, "Engine_Physics", initParams)) {
        return false;
    }

    X_ASSERT_NOT_NULL(env_.pPhysics);

    env_.pPhysics->registerVars();
    env_.pPhysics->registerCmds();

    return true;
}

bool XCore::InitNet(const CoreInitParams& initParams)
{
    if (!IntializeEngineModule(DLL_NET, "Engine_Network", initParams)) {
        return false;
    }

    X_ASSERT_NOT_NULL(env_.pNet);

    env_.pNet->registerVars();
    env_.pNet->registerCmds();

    if (!env_.pNet->init()) {
        X_ERROR("Font", "failed to init network system");
        return false;
    }

    return true;
}

bool XCore::InitVideo(const CoreInitParams& initParams)
{
    if (!IntializeEngineModule(DLL_VIDEO, "Engine_Video", initParams)) {
        return false;
    }

    X_ASSERT_NOT_NULL(env_.pVideoSys);

    env_.pVideoSys->registerVars();
    env_.pVideoSys->registerCmds();

    if (!env_.pVideoSys->init()) {
        X_ERROR("Video", "failed to init video system");
        return false;
    }

    return true;
}

bool XCore::InitScriptSys(const CoreInitParams& initParams)
{
    if (!IntializeEngineModule(DLL_SCRIPT, "Engine_Script", initParams)) {
        return false;
    }

    X_ASSERT_NOT_NULL(env_.pScriptSys);

    env_.pScriptSys->registerVars();
    env_.pScriptSys->registerCmds();

    if (!env_.pScriptSys->init()) {
        X_ERROR("Font", "failed to init script system");
        return false;
    }

    return true;
}

bool XCore::InitRenderSys(const CoreInitParams& initParams)
{
    if (initParams.bTesting) {
        if (!IntializeEngineModule(DLL_RENDER_NULL, "Engine_RenderNull", initParams)) {
            return false;
        }
    }
    else {
        if (!IntializeEngineModule(DLL_RENDER, "Engine_RenderDx12", initParams)) {
            return false;
        }
    }

    if (!env_.pRender) {
        return false;
    }

    const bool reverseZ = render::DEPTH_REVERSE_Z;

    if (initParams.bTesting) {
        // should never fail since it's null render system.
        // but it may get changed later that it can fail so check.
        if (!env_.pRender->init(nullptr, texture::Texturefmt::UNKNOWN, reverseZ)) {
            X_ERROR("Core", "Failed to init render system");
            return false;
        }
    }
    else {
        if (!pWindow_) {
            X_ERROR("Core", "Failed to init render system, no window");
            return false;
        }

        core::StopWatch timer;

        HWND hWnd = pWindow_->GetNativeWindow();

        // vars required pre init.
        env_.pRender->registerVars();

        if (!env_.pRender->init(hWnd, texture::Texturefmt::D32_FLOAT, reverseZ)) {
            X_ERROR("Core", "Failed to init render system");
            return false;
        }

        env_.pRender->registerCmds();

        X_LOG0("Core", "render init: ^6%gms", timer.GetMilliSeconds());
    }

    return true;
}

bool XCore::Init3DEngine(const CoreInitParams& initParams)
{
    if (!IntializeEngineModule(DLL_3D_ENGINE, "Engine_3DEngine", initParams)) {
        return false;
    }

    X_ASSERT_NOT_NULL(env_.p3DEngine);

    core::StopWatch timer;

    env_.p3DEngine->registerVars();
    env_.p3DEngine->registerCmds();

    if (!env_.p3DEngine->init()) {
        X_ERROR("Core", "Failed to init 3DEngine");
        return false;
    }

    X_LOG0("Core", "3DEngine init: ^6%gms", timer.GetMilliSeconds());

    return true;
}

bool XCore::InitGameDll(const CoreInitParams& initParams)
{
    if (!IntializeEngineModule(DLL_GAME_DLL, "Engine_Game", initParams)) {
        return false;
    }

    X_ASSERT_NOT_NULL(env_.pGame);

    env_.pGame->registerVars();
    env_.pGame->registerCmds();

    if (!env_.pGame->init()) {
        X_ERROR("Core", "Failed to init Game");
        return false;
    }

    return true;
}

// ->->->->->->->->->->->->->->->->->->->->->->->->->->->->->->->->->->->

void XCore::registerVars(const CoreInitParams& initParams)
{
    vars_.registerVars();

    assetLoader_.registerVars();

    if (!initParams.isCoreOnly() || initParams.basicConsole()) {
        core::ConsoleVarFunc del;

        del.Bind<XCore, &XCore::WindowPosVarChange>(this);
        vars_.getVarWinPosX()->SetOnChangeCallback(del);
        vars_.getVarWinPosY()->SetOnChangeCallback(del);

        del.Bind<XCore, &XCore::WindowSizeVarChange>(this);
        vars_.getVarWinWidth()->SetOnChangeCallback(del);
        vars_.getVarWinHeight()->SetOnChangeCallback(del);

        del.Bind<XCore, &XCore::WindowCustomFrameVarChange>(this);
        vars_.getVarWinCustomFrame()->SetOnChangeCallback(del);
    }
}

void XCore::registerCmds(void)
{
    using namespace core;

    assetLoader_.registerCmds();

    ADD_COMMAND_MEMBER("listProgramArgs", this, XCore, &XCore::Cmd_ListProgramArgs, VarFlag::SYSTEM,
        "Lists the processed command line arguments parsed to the program");

    ADD_COMMAND_MEMBER("listDisplayDevices", this, XCore, &XCore::Cmd_ListDisplayDevices, VarFlag::SYSTEM,
        "Lists the display devices <verbose>");

    // replay
    if (pReplaySys_) {
        ADD_COMMAND_MEMBER("replay_rec", this, XCore, &XCore::Cmd_ReplayRecord, VarFlag::SYSTEM,
            "Start recording a replay <filename>");
        ADD_COMMAND_MEMBER("replay_play", this, XCore, &XCore::Cmd_ReplayPlay, VarFlag::SYSTEM,
            "Start playing a replay <filename>");
        ADD_COMMAND_MEMBER("replay_stop", this, XCore, &XCore::Cmd_ReplayStop, VarFlag::SYSTEM,
            "Stop recording a replay");
    }
}


// ->->->->->->->->->->->->->->->->->->->->->->->->->->->->->->->->->->->

void XCore::RegisterAssertHandler(IAssertHandler* pErrorHandler)
{
    X_ASSERT(!env_.isRunning(), "Assert handlers must be registerd at statup")(env_.isRunning(), pErrorHandler);

    assertHandlers_.push_back(pErrorHandler);
}

void XCore::UnRegisterAssertHandler(IAssertHandler* pErrorHandler)
{
    assertHandlers_.remove(pErrorHandler);
}

void XCore::OnAssert(const core::SourceInfo& sourceInfo)
{
    for (auto* pHandler : assertHandlers_) {
        pHandler->OnAssert(sourceInfo);
    }
}

void XCore::OnAssertVariable(const core::SourceInfo& sourceInfo)
{
    for (auto* pHandler : assertHandlers_) {
        pHandler->OnAssertVariable(sourceInfo);
    }
}
