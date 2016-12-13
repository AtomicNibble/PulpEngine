#pragma once

#ifndef _X_CORE_H_
#define _X_CORE_H_

#include <ICore.h>
#include <IConsole.h>

#include "Timer.h"
#include "Util\Cpu.h"
#include "XProfile.h"

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

#include "Platform\DirectoryWatcher.h"
#include "IDirectoryWatcher.h"

#include "Containers\HashMap.h"

#include "Memory\AllocationPolicies\GrowingGenericAllocator.h"

#include "Threading\JobSystem.h"

// #include <vector>
// #include <map>

#ifdef WIN32
typedef void* WIN_HMODULE;
typedef void* WIN_HWND;
typedef void* WIN_HINSTANCE;
#else
typedef void* WIN_HMODULE;
typedef void* WIN_HWND;
typedef void* WIN_HINSTANCE;
#endif

struct IPotatoFactoryRegistryImpl;
struct IEngineModule;

X_NAMESPACE_DECLARE(core,
class xWindow;
class Console;
)


struct XCoreVars
{
	int core_event_debug;

	int win_x_pos;
	int win_y_pos;
	int win_width;
	int win_height;

};

extern XCoreVars g_coreVars;

typedef core::Logger<
core::LoggerNoFilterPolicy,
core::LoggerFullFormatPolicy,
core::LoggerDebuggerWritePolicy> VisualStudioLogger;

typedef core::Logger<
	core::LoggerVerbosityFilterPolicy,
	core::LoggerSimpleFormatPolicy,
	core::LoggerConsoleWritePolicy> ConsoleLogger;

#if X_ENABLE_MEMORY_DEBUG_POLICIES

typedef core::MemoryArena<
core::MallocFreeAllocator,
core::SingleThreadPolicy,
core::SimpleBoundsChecking,
core::SimpleMemoryTracking,
core::SimpleMemoryTagging
> GlobalArena;


typedef core::MemoryArena<
	core::GrowingGenericAllocator,
	core::SingleThreadPolicy,
	core::SimpleBoundsChecking,
	core::SimpleMemoryTracking,
	core::SimpleMemoryTagging
> StrArena;

#else

typedef core::MemoryArena<
	core::MallocFreeAllocator,
	core::SingleThreadPolicy,
	core::NoBoundsChecking,
	core::SimpleMemoryTracking,
	core::NoMemoryTagging
> GlobalArena;


typedef core::MemoryArena<
	core::GrowingGenericAllocator,
	core::SingleThreadPolicy,
	core::NoBoundsChecking,
	core::SimpleMemoryTracking,
	core::NoMemoryTagging
> StrArena;

#endif // !X_ENABLE_MEMORY_SIMPLE_TRACKING


class XCore :
	public ICore, 
	public core::IXHotReloadManager, 
	public core::XDirectoryWatcherListener,
	public ICoreEventListener
{
	static const size_t MAX_CMD_ARS = 16;
public:
	XCore();
	~XCore() X_FINAL;

	virtual bool Init(const SCoreInitParams &startupParams) X_FINAL;
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

	X_INLINE core::IProfileSys* GetIProfileSys(void) X_FINAL;
	X_INLINE core::IXDirectoryWatcher* GetDirWatcher(void) X_FINAL;
	X_INLINE core::IXHotReloadManager* GetHotReloadMan(void) X_FINAL;
	
	X_INLINE ICoreEventDispatcher* GetCoreEventDispatcher(void) X_FINAL;
	X_INLINE core::ILog* GetILog(void) X_FINAL;
	X_INLINE core::Crc32* GetCrc32(void) X_FINAL;
	X_INLINE core::CpuInfo*	GetCPUInfo(void) X_FINAL;
	
	X_INLINE core::xWindow* GetGameWindow(void) X_FINAL;

	X_INLINE SCoreGlobals* GetGlobalEnv(void) X_FINAL;
	X_INLINE core::MallocFreeAllocator* GetGlobalMalloc(void) X_FINAL;

	IPotatoFactoryRegistry* GetFactoryRegistry(void) const X_FINAL;

private:
	static SCoreGlobals env_;
	static core::MallocFreeAllocator malloc_;

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


	WIN_HMODULE LoadDynamiclibrary(const char *dllName) const;
	WIN_HMODULE LoadDLL(const char *dllName);

	bool IntializeEngineModule(const char *dllName, const char *moduleClassName,
		const SCoreInitParams &initParams);
	
	bool ParseCmdArgs(const wchar_t* pArgs);
	bool parseSeed(const Vec4i& seed);

	bool InitConsole(const SCoreInitParams &initParams);
	bool InitFileSys(const SCoreInitParams &startupParams);
	bool InitLogging(const SCoreInitParams &startupParams);
	bool InitInput(const SCoreInitParams &startupParams);
	bool InitFont(const SCoreInitParams &startupParams);
	bool InitSound(const SCoreInitParams &startupParams);
	bool InitScriptSys(const SCoreInitParams &startupParams);
	bool InitRenderSys(const SCoreInitParams &startupParams);
	bool Init3DEngine(const SCoreInitParams &startupParams);
	bool InitGameDll(const SCoreInitParams &startupParams);
	bool InitPhysics(const SCoreInitParams &startupParams);
	
	
	void CreateSystemVars(void);
	void AddIgnoredHotReloadExtensions(void);

	void Command_HotReloadListExts(core::IConsoleCmdArgs* Cmd);
	void Command_ListProgramArgs(core::IConsoleCmdArgs* Cmd);

	void HotReloadListExts(void);
	void ListProgramArgs(void);
	void LogSystemInfo(void) const;

	void Job_DirectoryWatcher(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData);
	void Job_OnFileChange(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData);
	void Job_ProcessInput(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData);
	void Job_PostInputFrame(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData);
	void Job_ConsoleUpdates(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData);

private:

	// IXHotReloadManager
	bool addfileType(core::IXHotReload* pHotReload, const char* extension) X_OVERRIDE;

	// ~IXHotReloadManager

	// XDirectoryWatcherListener
	bool OnFileChange(core::XDirectoryWatcher::Action::Enum action,
		const char* name, const char* oldName, bool isDirectory) X_OVERRIDE;
	// ~XDirectoryWatcherListener
	
	// ICoreEventListener
	void OnCoreEvent(CoreEvent::Enum event, UINT_PTR wparam, UINT_PTR lparam) X_OVERRIDE;
	// ~ICoreEventListener

private:
	// var callbacks.
	void WindowPosVarChange(core::ICVar* pVar);
	void WindowSizeVarChange(core::ICVar* pVar);
	void WindowCustomFrameVarChange(core::ICVar* pVar);

	struct ConverterModule : public core::ReferenceCounted<int32_t>
	{
		core::string dllName;
		core::string moduleClassName;
		IConverter* pConverter;
		std::shared_ptr<IConverterModule> pConModule;
	};

private:
	typedef core::Array<WIN_HMODULE> ModuleHandlesArr;
	typedef core::Array<std::shared_ptr<IEngineModule>> ModuleInterfacesArr;
	typedef core::Array<ConverterModule> ConverterModulesArr;
	typedef core::Array<IAssertHandler*> ArrsetHandlersArr;
	// I think i can just use stack strings, since all handlers are hard coded.
	typedef core::HashMap<const char* const, core::IXHotReload*> hotReloadMap;
	typedef core::Array<core::string> hotRelodIgnoreArr;
	typedef core::CmdArgs<1024, wchar_t> CmdArg;
	typedef core::FixedArray<CmdArg, MAX_CMD_ARS> CmdArgs;


	core::xWindow*				    pWindow_;
	core::Console*					pConsole_;

	VisualStudioLogger*				pVsLogger_;
	ConsoleLogger*					pConsoleLogger_;


	core::XTimer					time_;
	core::CpuInfo*					pCpuInfo_;
	core::Crc32*					pCrc32_;

	ModuleHandlesArr				moduleDLLHandles_;
	ModuleInterfacesArr				moduleInterfaces_;
	ConverterModulesArr				converterInterfaces_;
	ArrsetHandlersArr				assertHandlers_;

	core::XProfileSys				profileSys_;

	// Hot reload stuff
	core::XDirectoryWatcher			dirWatcher_;

	ICoreEventDispatcher*			pEventDispatcher_;

	hotReloadMap					hotReloadExtMap_;

#if X_DEBUG
	hotRelodIgnoreArr hotReloadIgnores_;
#endif // !X_DEBUG
	// ~Hotreload


	WIN_HWND		hWnd_;
	WIN_HINSTANCE	hInst_;

	SCoreInitParams initParams_;

	core::GrowingGenericAllocator strAlloc_;

	// args
	CmdArgs args_;

private:
	core::ICVar* var_win_pos_x;
	core::ICVar* var_win_pos_y;
	core::ICVar* var_win_width;
	core::ICVar* var_win_height;
	core::ICVar* var_win_custom_Frame;

	core::ICVar* var_profile;
};

X_NAMESPACE_BEGIN(core)



X_NAMESPACE_END


#include "Core.inl"

#endif // !_X_CORE_H_
