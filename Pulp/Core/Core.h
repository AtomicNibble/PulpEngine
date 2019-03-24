#pragma once

#ifndef _X_CORE_H_
#define _X_CORE_H_

#include <ICore.h>
#include <IConsole.h>
#include <IDirectoryWatcher.h>

#include "Vars\CoreVars.h"

#include "Timer.h"
#include "Util\Cpu.h"
#include "Profile\XProfile.h"

#include "Util\SourceInfo.h"
#include "Util\ReferenceCounted.h"
#include "String\StrRef.h"
#include "String\CmdArgs.h"

#include <Platform\Module.h>

#include "Containers\HashMap.h"

#include "Memory\AllocationPolicies\GrowingGenericAllocator.h"
#include "Memory\ThreadPolicies\MultiThreadPolicy.h"

#include <Assets\AssetLoader.h>

struct IEngineFactoryRegistryImpl;
struct IEngineModule;

X_NAMESPACE_DECLARE(core,
                    class Window;
                    class Console;
                    class XDirectoryWatcher;
                    class XCoreEventDispatcher;
                    class VisualStudioLogger;
                    class ConsoleLogger;
                    class ReplaySys;
)


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


class XCore : public ICore
    , public core::IDirectoryWatcherListener
    , public ICoreEventListener
{
    static const size_t MAX_CMD_ARS = 16;

    struct ConverterModule : public core::ReferenceCounted<int32_t>
    {
        core::string dllName;
        core::string moduleClassName;
        IConverter* pConverter;
        std::shared_ptr<IConverterModule> pConModule;
    };

    struct EngineModule : public core::ReferenceCounted<int32_t>
    {
        core::string dllName;
        core::string moduleClassName;
        std::shared_ptr<IEngineModule> pModule;
    };

    typedef core::Array<core::Module::Handle> ModuleHandlesArr;
    typedef core::Array<EngineModule> ModuleInterfacesArr;
    typedef core::Array<ConverterModule> ConverterModulesArr;
    typedef core::Array<IAssertHandler*> ArrsetHandlersArr;
    
    typedef core::Array<core::string> HotRelodIgnoreArr;
    typedef core::CmdArgs<1024, wchar_t> CmdArg;
    typedef core::FixedArray<CmdArg, MAX_CMD_ARS> CmdArgs;

public:
    XCore();
    ~XCore() X_FINAL;

    bool Init(CoreInitParams& startupParams) X_FINAL;
    bool InitAsyncWait(void) X_FINAL;
    void ShutDown(void); // not part of ICore currently
    void Release(void) X_FINAL;

    bool RunGameLoop(void) X_FINAL;
    const wchar_t* GetCommandLineArgForVarW(const wchar_t* pVarName) X_FINAL;
    bool IntializeLoadedEngineModule(const char* pDllName, const char* pModuleClassName) X_FINAL;
    bool IntializeLoadedConverterModule(const char* pDllName, const char* pModuleClassName,
        IConverterModule** pConvertModuleOut = nullptr, IConverter** pConverterInstance = nullptr) X_FINAL;
    bool FreeConverterModule(IConverterModule* pConvertModule) X_FINAL;

    X_INLINE core::ITimer* GetITimer(void) X_FINAL;
    X_INLINE input::IInput* GetIInput(void) X_FINAL;
    X_INLINE core::IConsole* GetIConsole(void) X_FINAL;
    X_INLINE core::IFileSys* GetIFileSys(void) X_FINAL;
    X_INLINE sound::ISound* GetISound(void) X_FINAL;
    X_INLINE engine::I3DEngine* Get3DEngine(void) X_FINAL;
    X_INLINE script::IScriptSys* GetISscriptSys(void) X_FINAL;
    X_INLINE render::IRender* GetIRender(void) X_FINAL;
    X_INLINE font::IFontSys* GetIFontSys(void) X_FINAL;
    X_INLINE core::V2::JobSystem* GetJobSystem(void) X_FINAL;
    X_INLINE physics::IPhysics* GetPhysics(void) X_FINAL;

    X_INLINE core::profiler::IProfiler* GetProfiler(void) X_FINAL;
    core::IDirectoryWatcher* GetDirWatcher(void) X_FINAL;

    ICoreEventDispatcher* GetCoreEventDispatcher(void) X_FINAL;
    X_INLINE core::ILog* GetILog(void) X_FINAL;
    X_INLINE core::Crc32* GetCrc32(void) X_FINAL;
    X_INLINE const core::CpuInfo* GetCPUInfo(void) X_FINAL;

    X_INLINE core::Window* GetGameWindow(void) X_FINAL;
    X_INLINE core::AssetLoader* GetAssetLoader(void) X_FINAL;

    X_INLINE CoreGlobals* GetGlobalEnv(void) X_FINAL;
    X_INLINE core::MallocFreeAllocator* GetGlobalMalloc(void) X_FINAL;

    IEngineFactoryRegistry* GetFactoryRegistry(void) const X_FINAL;

public:
    void RegisterAssertHandler(IAssertHandler* pErrorHandler) X_FINAL;
    void UnRegisterAssertHandler(IAssertHandler* pErrorHandler) X_FINAL;
    void OnAssert(const core::SourceInfo& sourceInfo) X_FINAL;
    void OnAssertVariable(const core::SourceInfo& sourceInfo) X_FINAL;

    void OnFatalError(const char* pFormat, va_list args) X_FINAL;

private:
    void toggleFullscreen(void);

    bool PumpMessages(void);
    bool Update(void);
    void RenderBegin(core::FrameData& frameData);
    void RenderEnd(core::FrameData& frameData);

    core::Module::Handle LoadDLL(const char* pDllName);

    bool IntializeEngineModule(const char* pDllName, const char* pModuleClassName, const CoreInitParams& initParams);

    bool ParseCmdArgs(CoreInitParams& initParams);
    bool parseSeed(Vec4i seed);

    bool InitConsole(const CoreInitParams& initParams);
    bool InitFileSys(const CoreInitParams& startupParams);
    bool InitLogging(const CoreInitParams& startupParams);
    bool InitFileLogging(const CoreInitParams& startupParams);
    bool InitInput(const CoreInitParams& startupParams);
    bool InitFont(const CoreInitParams& startupParams);
    bool InitSound(const CoreInitParams& startupParams);
    bool InitScriptSys(const CoreInitParams& startupParams);
    bool InitRenderSys(const CoreInitParams& startupParams);
    bool Init3DEngine(const CoreInitParams& startupParams);
    bool InitGameDll(const CoreInitParams& startupParams);
    bool InitPhysics(const CoreInitParams& startupParams);
    bool InitNet(const CoreInitParams& startupParams);
    bool InitVideo(const CoreInitParams& startupParams);

    void registerVars(const CoreInitParams& initParams);
    void registerCmds(void);

    void Cmd_ListProgramArgs(core::IConsoleCmdArgs* pCmd);
    void Cmd_ListDisplayDevices(core::IConsoleCmdArgs* pCmd);
    void Cmd_ReplayRecord(core::IConsoleCmdArgs* pCmd);
    void Cmd_ReplayPlay(core::IConsoleCmdArgs* pCmd);
    void Cmd_ReplayStop(core::IConsoleCmdArgs* pCmd);

    void ListProgramArgs(void);
    void LogSystemInfo(void) const;
    void ListDisplayDevices(bool verbose) const;

    void Job_DirectoryWatcher(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData);

private:
    // IDirectoryWatcherListener
    bool OnFileChange(core::IDirectoryWatcher::Action::Enum action,
        const char* pName, const char* pOldName, bool isDirectory) X_FINAL;
    // ~IDirectoryWatcherListener

    // ICoreEventListener
    void OnCoreEvent(const CoreEventData& ed) X_FINAL;
    // ~ICoreEventListener

private:
    // var callbacks.
    void WindowPosVarChange(core::ICVar* pVar);
    void WindowSizeVarChange(core::ICVar* pVar);
    void WindowCustomFrameVarChange(core::ICVar* pVar);

private:
    static CoreGlobals env_;

private:
    core::CoreVars vars_;
    core::Window* pWindow_;
    core::Console* pConsole_;

    core::LoggerBase* pVsLogger_;
    core::LoggerBase* pConsoleLogger_;
    core::LoggerBase* pFileLogger_;

    core::XTimer time_;
    core::CpuInfo* pCpuInfo_;
    core::Crc32* pCrc32_;
    core::ReplaySys* pReplaySys_;

    int32_t numFrames_;

    ModuleHandlesArr moduleDLLHandles_;
    ModuleInterfacesArr moduleInterfaces_;
    ConverterModulesArr converterInterfaces_;
    ArrsetHandlersArr assertHandlers_;

#if X_ENABLE_PROFILER
    core::profiler::XProfileSys* pProfiler_;
#endif // !X_ENABLE_PROFILER

    // Hot reload stuff
    core::XDirectoryWatcher* pDirWatcher_;

    core::XCoreEventDispatcher* pCoreEventDispatcher_;
    // ~Hotreload

    CoreInitParams initParams_;

    core::GrowingGenericAllocator strAlloc_;
    core::MallocFreeAllocator malloc_;

    // args
    CmdArgs args_;

    core::UniquePointer<core::AssetLoader> assetLoader_;

    // Telem
    core::UniquePointer<uint8_t[]> telemBuf_;
};

X_NAMESPACE_BEGIN(core)

X_NAMESPACE_END

#include "Core.inl"

#endif // !_X_CORE_H_
