#pragma once

#ifndef _X_CORE_H_
#define _X_CORE_H_

#include <ICore.h>
#include <IConsole.h>

#include "Timer.h"
#include "Util\Cpu.h"
#include "XProfile.h"

#include "Util\SourceInfo.h"
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


class XCore :
	public ICore, 
	public core::IXHotReloadManager, 
	public core::XDirectoryWatcherListener,
	public ICoreEventListener
{
	static const size_t MAX_CMD_ARS = 16;
public:
	XCore();
	~XCore() X_OVERRIDE;

	virtual bool Init(const SCoreInitParams &startupParams) X_OVERRIDE;
	virtual void ShutDown(void); // not part of ICore currently
	virtual void Release(void) X_OVERRIDE;

	virtual bool RunGameLoop(void) X_FINAL;

	virtual const wchar_t* GetCommandLineArgForVarW(const wchar_t* pVarName) X_OVERRIDE;

	virtual bool IntializeLoadedEngineModule(const char* pDllName, const char* pModuleClassName) X_OVERRIDE;


	core::ITimer		*GetITimer(void) X_OVERRIDE{ return env_.pTimer; }
	input::IInput		*GetIInput(void) X_OVERRIDE{ return env_.pInput; }
	core::IConsole		*GetIConsole(void) X_OVERRIDE{ return env_.pConsole; }
	core::IFileSys		*GetIFileSys(void) X_OVERRIDE{ return env_.pFileSys; }
	sound::ISound		*GetISound(void) X_OVERRIDE{ return env_.pSound; }
	script::IScriptSys  *GetISscriptSys(void) X_OVERRIDE{ return env_.pScriptSys; }
	render::IRender		*GetIRender(void) X_OVERRIDE{ return env_.pRender; }
	font::IXFontSys		*GetIFontSys(void) X_OVERRIDE{ return env_.pFont; }
//	core::IJobSystem	*GetJobSystem(void) X_OVERRIDE{ return env_.pJobSys; }
	physics::IPhysics*	 GetPhysics(void) X_OVERRIDE { return env_.pPhysics; };

	core::IProfileSys	*GetIProfileSys(void) X_OVERRIDE{ return &profileSys_; }
	core::IXDirectoryWatcher *GetDirWatcher(void) X_OVERRIDE{ return env_.pDirWatcher; }
	core::IXHotReloadManager*   GetHotReloadMan(void) X_OVERRIDE{ return this; };

	ICoreEventDispatcher* GetCoreEventDispatcher(void) X_OVERRIDE{ return pEventDispatcher_; }
	core::ILog			*GetILog(void) X_OVERRIDE{ return env_.pLog; };
	core::Crc32			*GetCrc32(void) X_OVERRIDE;
	core::CpuInfo*	GetCPUInfo(void) X_OVERRIDE { return pCpuInfo_; };


	core::xWindow* GetGameWindow(void) X_OVERRIDE { return pWindow_; }

	virtual IPotatoFactoryRegistry* GetFactoryRegistry(void) const X_OVERRIDE;


	virtual SCoreGlobals* GetGlobalEnv(void) X_OVERRIDE{ return &env_; }
	virtual core::MallocFreeAllocator* GetGlobalMalloc(void) X_OVERRIDE{ return &malloc_; }

	static SCoreGlobals env_;
	static core::MallocFreeAllocator malloc_;

public:
	virtual void RegisterAssertHandler(IAssertHandler* errorHandler) X_OVERRIDE;
	virtual void UnRegisterAssertHandler(IAssertHandler* errorHandler) X_OVERRIDE;
	virtual void OnAssert(const core::SourceInfo& sourceInfo) X_OVERRIDE;
	virtual void OnAssertVariable(const core::SourceInfo& sourceInfo) X_OVERRIDE;

	virtual void OnFatalError(const char* format, va_list args) X_OVERRIDE;

private:
	bool PumpMessages(void);
	bool Update(void);
	void RenderBegin(void);
	void RenderEnd(void);


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

	void Command_HotReloadListExts(core::IConsoleCmdArgs* Cmd);
	void Command_ListProgramArgs(core::IConsoleCmdArgs* Cmd);

	void HotReloadListExts(void);
	void ListProgramArgs(void);
	void LogSystemInfo(void) const;

	void Job_DirectoryWatcher(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData);
	void Job_OnFileChange(core::V2::JobSystem& jobSys, size_t threadIdx, core::V2::Job* pJob, void* pData);


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
//	GlobalArena						arena_;

	core::xWindow*				    pWindow_;
	core::Console*					pConsole_;

	VisualStudioLogger*				pVsLogger_;
	ConsoleLogger*					pConsoleLogger_;


	core::XTimer					time_;
	core::CpuInfo*					pCpuInfo_;

	core::Array<WIN_HMODULE>		moduleDLLHandles_;
	core::Array<std::shared_ptr<IEngineModule>>    
									moduleInterfaces_;
	core::Array<IAssertHandler*>	assertHandlers_;

	core::XProfileSys				profileSys_;

	// Hot reload stuff
	core::XDirectoryWatcher			dirWatcher_;

	ICoreEventDispatcher*			pEventDispatcher_;

	// I think i can just use stack strings, since all handlers are hard coded.
	typedef core::HashMap<const char* const, core::IXHotReload*> hotReloadMap;

	hotReloadMap					hotReloadExtMap_;

#if X_DEBUG
	typedef core::Array<core::string> hotRelodIgnoreList;

	hotRelodIgnoreList hotReloadIgnores_;
#endif // !X_DEBUG
	// ~Hotreload


	WIN_HWND		hWnd_;
	WIN_HINSTANCE	hInst_;

	SCoreInitParams initParams_;

	core::GrowingGenericAllocator	strAlloc_;

	// args
	typedef core::CmdArgs<1024, wchar_t> CmdArg;

	size_t numArgs_;
	CmdArg args_[MAX_CMD_ARS];

public:
	// All the vars
	void WindowPosVarChange(core::ICVar* pVar);
	void WindowSizeVarChange(core::ICVar* pVar);
	void WindowCustomFrameVarChange(core::ICVar* pVar);


	core::ICVar* var_win_pos_x;
	core::ICVar* var_win_pos_y;
	core::ICVar* var_win_width;
	core::ICVar* var_win_height;
	core::ICVar* var_win_custom_Frame;

	core::ICVar* var_profile;

};

X_NAMESPACE_BEGIN(core)



X_NAMESPACE_END


#endif // !_X_CORE_H_
