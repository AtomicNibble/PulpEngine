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

// Logging
#include "Logging\Logger.h"
#include "Logging\FilterPolicies\LoggerNoFilterPolicy.h"
#include "Logging\FilterPolicies\LoggerVerbosityFilterPolicy.h"
#include "Logging\FormatPolicies\LoggerExtendedFormatPolicy.h"
#include "Logging\FormatPolicies\LoggerSimpleFormatPolicy.h"
#include "Logging\FormatPolicies\LoggerFullFormatPolicy.h"
#include "Logging\WritePolicies\LoggerDebuggerWritePolicy.h"
#include "Logging\WritePolicies\LoggerConsoleWritePolicy.h"

#include <Platform\Module.h>

#include "Containers\HashMap.h"

#include "Memory\AllocationPolicies\GrowingGenericAllocator.h"
#include "Memory\ThreadPolicies\MultiThreadPolicy.h"

#include <Assets\AssetLoader.h>

struct IEngineFactoryRegistryImpl;
struct IEngineModule;

X_NAMESPACE_DECLARE(core,
                    class xWindow;
                    class Console;
                    class XDirectoryWatcher;)

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

class XCore : public ICore
    , public core::IXHotReloadManager
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

    typedef core::Array<core::Module::Handle> ModuleHandlesArr;
    typedef core::Array<std::shared_ptr<IEngineModule>> ModuleInterfacesArr;
    typedef core::Array<ConverterModule> ConverterModulesArr;
    typedef core::Array<IAssertHandler*> ArrsetHandlersArr;
    // I think i can just use stack strings, since all handlers are hard coded.
    typedef core::HashMap<const char* const, core::IXHotReload*> HotReloadMap;
    typedef core::Array<core::string> HotRelodIgnoreArr;
    typedef core::CmdArgs<1024, wchar_t> CmdArg;
    typedef core::FixedArray<CmdArg, MAX_CMD_ARS> CmdArgs;

public:
    XCore();
    ~XCore() X_FINAL;

    virtual bool Init(const CoreInitParams& startupParams) X_FINAL;
    virtual bool InitAsyncWait(void) X_FINAL;
    virtual void ShutDown(void); // not part of ICore currently
    virtual void Release(void) X_FINAL;

    virtual bool RunGameLoop(void) X_FINAL;
    virtual const wchar_t* GetCommandLineArgForVarW(const wchar_t* pVarName) X_FINAL;
    virtual bool IntializeLoadedEngineModule(const char* pDllName, const char* pModuleClassName) X_FINAL;
    virtual bool IntializeLoadedConverterModule(const char* pDllName, const char* pModuleClassName,
        IConverterModule** pConvertModuleOut = nullptr, IConverter** pConverterInstance = nullptr) X_FINAL;
    virtual bool FreeConverterModule(IConverterModule* pConvertModule) X_FINAL;

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
    X_INLINE core::IXHotReloadManager* GetHotReloadMan(void) X_FINAL;

    X_INLINE ICoreEventDispatcher* GetCoreEventDispatcher(void) X_FINAL;
    X_INLINE core::ILog* GetILog(void) X_FINAL;
    X_INLINE core::Crc32* GetCrc32(void) X_FINAL;
    X_INLINE const core::CpuInfo* GetCPUInfo(void) X_FINAL;

    X_INLINE core::xWindow* GetGameWindow(void) X_FINAL;
    X_INLINE core::AssetLoader* GetAssetLoader(void) X_FINAL;

    X_INLINE SCoreGlobals* GetGlobalEnv(void) X_FINAL;
    X_INLINE core::MallocFreeAllocator* GetGlobalMalloc(void) X_FINAL;

    IEngineFactoryRegistry* GetFactoryRegistry(void) const X_FINAL;

public:
    virtual void RegisterAssertHandler(IAssertHandler* errorHandler) X_FINAL;
    virtual void UnRegisterAssertHandler(IAssertHandler* errorHandler) X_FINAL;
    virtual void OnAssert(const core::SourceInfo& sourceInfo) X_FINAL;
    virtual void OnAssertVariable(const core::SourceInfo& sourceInfo) X_FINAL;

    virtual void OnFatalError(const char* format, va_list args) X_FINAL;

private:
    bool PumpMessages(void);
    bool Update(void);
    void RenderBegin(core::FrameData& frameData);
    void RenderEnd(core::FrameData& frameData);

    core::Module::Handle LoadDLL(const char* pDllName);

    bool IntializeEngineModule(const char* pDllName, const char* pModuleClassName, const CoreInitParams& initParams);

    bool ParseCmdArgs(const wchar_t* pArgs);
    bool parseSeed(Vec4i seed);

    bool InitConsole(const CoreInitParams& initParams);
    bool InitFileSys(const CoreInitParams& startupParams);
    bool InitLogging(const CoreInitParams& startupParams);
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

    void AddIgnoredHotReloadExtensions(void);

    void Command_HotReloadListExts(core::IConsoleCmdArgs* Cmd);
    void Command_ListProgramArgs(core::IConsoleCmdArgs* Cmd);

    void HotReloadListExts(void);
    void ListProgramArgs(void);
    void LogSystemInfo(void) const;

    void Job_DirectoryWatcher(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData);
    void Job_OnFileChange(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData);
    void Job_PostInputFrame(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData);
    void Job_ConsoleUpdates(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData);

private:
    // IXHotReloadManager
    bool addfileType(core::IXHotReload* pHotReload, const char* extension) X_FINAL;
    void unregisterListener(core::IXHotReload* pHotReload) X_FINAL;

    // ~IXHotReloadManager

    // IDirectoryWatcherListener
    bool OnFileChange(core::IDirectoryWatcher::Action::Enum action,
        const char* name, const char* oldName, bool isDirectory) X_OVERRIDE;
    // ~IDirectoryWatcherListener

    // ICoreEventListener
    void OnCoreEvent(CoreEvent::Enum event, UINT_PTR wparam, UINT_PTR lparam) X_OVERRIDE;
    // ~ICoreEventListener

private:
    // var callbacks.
    void WindowPosVarChange(core::ICVar* pVar);
    void WindowSizeVarChange(core::ICVar* pVar);
    void WindowCustomFrameVarChange(core::ICVar* pVar);

private:
    static SCoreGlobals env_;

private:
    core::CoreVars vars_;
    core::xWindow* pWindow_;
    core::Console* pConsole_;

    VisualStudioLogger* pVsLogger_;
    ConsoleLogger* pConsoleLogger_;

    core::XTimer time_;
    core::CpuInfo* pCpuInfo_;
    core::Crc32* pCrc32_;

    ModuleHandlesArr moduleDLLHandles_;
    ModuleInterfacesArr moduleInterfaces_;
    ConverterModulesArr converterInterfaces_;
    ArrsetHandlersArr assertHandlers_;

#if X_ENABLE_PROFILER
    core::profiler::XProfileSys* pProfiler_;
#endif // !X_ENABLE_PROFILER

    // Hot reload stuff
    core::XDirectoryWatcher* pDirWatcher_;

    ICoreEventDispatcher* pEventDispatcher_;

    HotReloadMap hotReloadExtMap_;

#if X_DEBUG
    HotRelodIgnoreArr hotReloadIgnores_;
#endif // !X_DEBUG \
    // ~Hotreload

    CoreInitParams initParams_;

    core::GrowingGenericAllocator strAlloc_;
    core::MallocFreeAllocator malloc_;

    // args
    CmdArgs args_;

    core::AssetLoader assetLoader_;
};

X_NAMESPACE_BEGIN(core)

X_NAMESPACE_END

#include "Core.inl"

#endif // !_X_CORE_H_
