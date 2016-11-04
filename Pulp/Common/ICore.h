#pragma once

#ifndef _X_CORE_I_H_
#define _X_CORE_I_H_

#include <Core\Platform.h>
#include <Traits\FunctionSignatureTraits.h>
// #include <Util/SourceInfo.h>

#include <Math\XVector.h>

// defaults.
// #include <Memory\MemoryArenaBase.h>

#ifdef IPCORE_EXPORTS
#define IPCORE_API DLL_EXPORT
#else
#define IPCORE_API DLL_IMPORT
#endif

#include <ILog.h>
#include <IProfile.h>

struct ICore;
struct IPotatoFactoryRegistry;

X_NAMESPACE_DECLARE(input, struct IInput)
X_NAMESPACE_DECLARE(core, 
struct ILog; 
struct ITimer; 
struct SourceInfo; 
struct IConsole; 
struct IFileSys;
struct IJobSystem;
struct IXHotReloadManager;
struct IXDirectoryWatcher;
struct FrameData;
class XProfileScope;
class Crc32;
class CpuInfo;
class Console;
class xWindow;
class MallocFreeAllocator;
class MemoryArenaBase;

namespace V2 {
	class JobSystem;
	struct Job;
}
)
X_NAMESPACE_DECLARE(font, struct IFontSys; struct IFont)
X_NAMESPACE_DECLARE(sound, struct ISound)
X_NAMESPACE_DECLARE(script, struct IScriptSys)
X_NAMESPACE_DECLARE(render, 
struct IRender;
)
X_NAMESPACE_DECLARE(engine,
struct I3DEngine;
)
X_NAMESPACE_DECLARE(game,
struct IGame;
)
X_NAMESPACE_DECLARE(physics,
	struct IPhysics;
)

X_USING_NAMESPACE;




typedef void(*pProfileScopeCall)(class core::XProfileScope* pProScope);

struct CoreEvent
{
	enum Enum
	{
		CHANGE_FOCUS = 10,
		MOVE = 11,
		RESIZE = 12,
		ACTIVATE = 13,

		LEVEL_LOAD_START_LOADINGSCREEN,
		LEVEL_LOAD_START,
		LEVEL_LOAD_END,
		LEVEL_UNLOAD,
		LEVEL_POST_UNLOAD,

		GAME_POST_INIT,
	//	GAME_POST_INIT_DONE,
		SHUTDOWN,
		TOGGLE_FULLSCREEN,
	//	POST_3D_RENDERING_START,
	//	POST_3D_RENDERING_END,
		USER = 0x1000,
	};

	// MEOW!
	static const char* toString(Enum event) {

		switch (event)
		{
			case Enum::CHANGE_FOCUS:
				return "CHANGE_FOCUS";
			case Enum::MOVE:
				return "MOVE";
			case Enum::RESIZE:
				return "RESIZE";
			case Enum::ACTIVATE:
				return "ACTIVATE";
			case Enum::GAME_POST_INIT:
				return "GAME_POST_INIT";
			case Enum::SHUTDOWN:
				return "SHUTDOWN";
			case Enum::TOGGLE_FULLSCREEN:
				return "TOGGLE_FULLSCREEN";

			case Enum::LEVEL_LOAD_START_LOADINGSCREEN:
				return "LEVEL_LOAD_START_LOADINGSCREEN";
			case Enum::LEVEL_LOAD_START:
				return "LEVEL_LOAD_START";
			case Enum::LEVEL_LOAD_END:
				return "LEVEL_LOAD_END";
			case Enum::LEVEL_UNLOAD:
				return "LEVEL_UNLOAD";
			case Enum::LEVEL_POST_UNLOAD:
				return "LEVEL_POST_UNLOAD";

			case Enum::USER:
				return "USER";

			default:
#if X_DEBUG
				X_ASSERT_UNREACHABLE();
				return "";

#else
				X_NO_SWITCH_DEFAULT;
#endif // !X_DEBUG

		}
	}
};

// Description:
//	 Interface used for getting notified when a system event occurs.
struct ICoreEventListener
{
	virtual ~ICoreEventListener(){}
	virtual void OnCoreEvent(CoreEvent::Enum event, UINT_PTR wparam, UINT_PTR lparam) X_ABSTRACT;
};

// Description:
//	 Structure used for getting notified when a system event occurs.
struct ICoreEventDispatcher
{
	virtual ~ICoreEventDispatcher(){}
	virtual bool RegisterListener(ICoreEventListener* pListener) X_ABSTRACT;
	virtual bool RemoveListener(ICoreEventListener* pListener) X_ABSTRACT;

	virtual void OnCoreEvent(CoreEvent::Enum event, UINT_PTR wparam, UINT_PTR lparam) X_ABSTRACT;
};

struct IAssertHandler
{
	virtual ~IAssertHandler() {}
	virtual void OnAssert(const core::SourceInfo& sourceInfo) X_ABSTRACT;
	virtual void OnAssertVariable(const core::SourceInfo& sourceInfo) X_ABSTRACT;
};


// Description:
//  Structure passed to Init method of ISystem interface.
struct SCoreInitParams
{
	void* hInstance;								
	void* hWnd;
	const wchar_t* pCmdLine;

//	core::LoggerBase* pLog;
	core::Console* pConsoleWnd;
	core::MemoryArenaBase* pCoreArena;

	bool bVsLog;
	bool bConsoleLog;
	bool bTesting;
	bool bSkipInput;
	bool bSkipSound;
	bool bCoreOnly;
	bool bEnableBasicConsole; // when in core only mode, optional enable a basic console.
	bool bEnableJobSystem;
	bool bLoadSymbols;
	bool bFileSysWorkingDir;

	Vec4i seed;

	const bool isCoreOnly(void) const {
		return bCoreOnly;
	}

	const bool basicConsole(void) const {
		return isCoreOnly() && bEnableBasicConsole;
	}

	const bool jobSystemEnabled(void) const {
		return bEnableJobSystem;
	}

	const bool loadSymbols(void) const {
		return bLoadSymbols;
	}

	const bool FileSysWorkingDir(void) const {
		return bFileSysWorkingDir;
	}

	SCoreInitParams() :
		hInstance(nullptr),
		hWnd(nullptr),
		pCmdLine(nullptr),
		bSkipInput(false),
		bSkipSound(false),
		pConsoleWnd(nullptr),
		bTesting(false),
		bCoreOnly(false),
		bEnableBasicConsole(false),
		bEnableJobSystem(true),
		bLoadSymbols(true),
		bFileSysWorkingDir(false),

#if X_SUPER == 0
		bConsoleLog(true),
#else
		bConsoleLog(false),
#endif
		pCoreArena(nullptr),

#if X_DEBUG 
		bVsLog(true)
#else
		bVsLog(false)
#endif
	{}
};


struct SCoreGlobals // obbject is zerod on start.
{
	X_DECLARE_ENUM8(State)(STARTING, RUNNING, CLOSING);

	ICore*						pCore;
	input::IInput*				pInput;
	core::ITimer*				pTimer;
	core::IConsole*				pConsole;
	core::IFileSys*				pFileSys;
	font::IFontSys*				pFontSys;
	sound::ISound*				pSound;
	core::ILog*					pLog;
//	core::IProfileSys*			pProfileSys;
	script::IScriptSys*			pScriptSys;
	render::IRender*			pRender;
	engine::I3DEngine*			p3DEngine;
	physics::IPhysics*			pPhysics;
	game::IGame*				pGame;
//	core::IJobSystem*			pJobSys;
	core::V2::JobSystem*		pJobSys;


	core::IXDirectoryWatcher*   pDirWatcher;
	core::IXHotReloadManager*   pHotReload;

//	core::MallocFreeAllocator*  pMalloc;
	core::MemoryArenaBase*		pArena;
	core::MemoryArenaBase*		pStrArena;

	uint32_t					uMainThreadId;		

	
	pProfileScopeCall			profileScopeStart;
	pProfileScopeCall			profileScopeEnd;

	X_INLINE const bool isStarting(void) const {
		return state_ != State::STARTING;
	}
	X_INLINE const bool isRunning(void) const {
		return state_ == State::RUNNING;
	}
	X_INLINE const bool isClosing(void) const {
		return state_ == State::CLOSING;
	}

	X_INLINE const bool IsClient(void) const {
		return client_;
	}

	X_INLINE const bool IsDedicated(void) const
	{
#if defined(X_DEDICATED_SERVER)
		return true;
#else
		return dedicated_;
#endif // defined(X_DEDICATED_SERVER)
	}

	X_INLINE const bool IsProfilerEnabled(void) const {
		return profilerEnabled_;
	}


protected:
	friend class XCore;

	bool client_;
	bool dedicated_;			// Engine is in dedicated 
	bool profilerEnabled_;
	State::Enum state_; 
};


struct ICore
{
	virtual ~ICore(){}

	virtual bool Init(const SCoreInitParams &startupParams) X_ABSTRACT;
	virtual void Release(void) X_ABSTRACT;

	// Update all the systems.
	virtual bool RunGameLoop(void) X_ABSTRACT;

	// cmd-line util
	virtual const wchar_t* GetCommandLineArgForVarW(const wchar_t* pVarName) X_ABSTRACT;

	virtual bool IntializeLoadedEngineModule(const char* pDllName, const char* pModuleClassName) X_ABSTRACT;
	virtual bool IntializeLoadedConverterModule(const char* pDllName, const char* pModuleClassName) X_ABSTRACT;

	virtual IPotatoFactoryRegistry* GetFactoryRegistry(void) const X_ABSTRACT;

	virtual SCoreGlobals* GetGlobalEnv(void) X_ABSTRACT;
	virtual core::MallocFreeAllocator* GetGlobalMalloc(void) X_ABSTRACT;

	virtual core::ITimer* GetITimer(void) X_ABSTRACT;
	virtual input::IInput* GetIInput(void) X_ABSTRACT;
	virtual core::IConsole* GetIConsole(void) X_ABSTRACT;
	virtual core::IFileSys* GetIFileSys(void) X_ABSTRACT;
	virtual sound::ISound* GetISound(void) X_ABSTRACT;
	virtual engine::I3DEngine* Get3DEngine(void) X_ABSTRACT;
	virtual script::IScriptSys* GetISscriptSys(void) X_ABSTRACT;
	virtual render::IRender* GetIRender(void) X_ABSTRACT;
	virtual font::IFontSys* GetIFontSys(void) X_ABSTRACT;
	virtual core::V2::JobSystem* GetJobSystem(void) X_ABSTRACT;
	virtual physics::IPhysics* GetPhysics(void) X_ABSTRACT;
	virtual core::IProfileSys* GetIProfileSys(void) X_ABSTRACT;
	virtual core::ILog* GetILog(void) X_ABSTRACT;
	virtual core::IXDirectoryWatcher* GetDirWatcher(void) X_ABSTRACT;
	virtual core::IXHotReloadManager* GetHotReloadMan(void) X_ABSTRACT;
	virtual ICoreEventDispatcher* GetCoreEventDispatcher(void) X_ABSTRACT;
	virtual core::Crc32* GetCrc32(void) X_ABSTRACT;
	virtual core::CpuInfo* GetCPUInfo(void) X_ABSTRACT;
	virtual core::xWindow* GetGameWindow(void) X_ABSTRACT;

	// ~Assert
	virtual void RegisterAssertHandler(IAssertHandler* errorHandler) X_ABSTRACT;
	virtual void UnRegisterAssertHandler(IAssertHandler* errorHandler) X_ABSTRACT;
	virtual void OnAssert(const core::SourceInfo& sourceInfo) X_ABSTRACT;
	virtual void OnAssertVariable(const core::SourceInfo& sourceInfo) X_ABSTRACT;

	virtual void OnFatalError(const char* format, va_list args) X_ABSTRACT;
};



extern SCoreGlobals* gEnv;
extern core::MallocFreeAllocator* gMalloc;

#if defined(X_CVAR_NO_DESCRIPTION) && X_CVAR_NO_DESCRIPTION == 1
#define CVARTEXT(_Desc)	0
#else
#define CVARTEXT(_Desc)	_Desc
#endif


#define ADD_CVAR_REF(name,_var,_def_val,_Min,_Max,_flags,_Desc)			gEnv->pConsole->Register(name, &(_var), (_def_val), (_Min), (_Max), (_flags), CVARTEXT(_Desc))
#define ADD_CVAR_REF_VEC3(name,_var,_def_val,_flags,_Desc)				gEnv->pConsole->Register(name, &(_var), (_def_val), (_flags), CVARTEXT(_Desc))
#define ADD_CVAR_REF_COL(name,_var,_def_val,_flags,_Desc)				gEnv->pConsole->Register(name, &(_var), (_def_val), (_flags), CVARTEXT(_Desc))
#define ADD_CVAR_REF_NO_NAME(_var,_def_val,_Min,_Max,_flags,_Desc)		gEnv->pConsole->Register(X_STRINGIZE(_var), &(_var), (_def_val), (_Min), (_Max), (_flags), CVARTEXT(_Desc))
#define ADD_CVAR_REF_COL_NO_NAME(_var,_def_val,_flags,_Desc)			gEnv->pConsole->Register(X_STRINGIZE(_var), &(_var), (_def_val), (_flags), CVARTEXT(_Desc))
#define ADD_CVAR_REF_VEC3_NO_NAME(_var,_def_val,_flags,_Desc)			gEnv->pConsole->Register(X_STRINGIZE(_var), &(_var), (_def_val), (_flags), CVARTEXT(_Desc))

#define ADD_CVAR_INT(_name,_val,_Min,_Max,_flags,_Desc)					gEnv->pConsole->RegisterInt(_name, (_val), (_Min), (_Max), (_flags), CVARTEXT(_Desc))
#define ADD_CVAR_FLOAT(_name,_val,_Min,_Max,_flags,_Desc)				gEnv->pConsole->RegisterFloat(_name, (_val), (_Min), (_Max), (_flags), CVARTEXT(_Desc))
#define ADD_CVAR_STRING(_name,_val,_flags,_Desc)						gEnv->pConsole->RegisterString(_name, (_val), (_flags), CVARTEXT(_Desc))

#define ADD_COMMAND(_name,_func,_flags,_Desc) \
X_MULTILINE_MACRO_BEGIN \
core::ConsoleCmdFunc X_PP_UNIQUE_NAME(del); \
X_PP_UNIQUE_NAME(del).Bind<_func>(); \
gEnv->pConsole->AddCommand(_name, X_PP_UNIQUE_NAME(del), (_flags), CVARTEXT(_Desc)); \
X_MULTILINE_MACRO_END

#define ADD_COMMAND_MEMBER(_name, __inst, __class, _func,_flags,_Desc) \
X_MULTILINE_MACRO_BEGIN \
core::ConsoleCmdFunc X_PP_UNIQUE_NAME(del); \
X_PP_UNIQUE_NAME(del).Bind<__class, _func>(__inst); \
gEnv->pConsole->AddCommand(_name, X_PP_UNIQUE_NAME(del), (_flags), CVARTEXT(_Desc)); \
X_MULTILINE_MACRO_END



// All logging done via this.
#include "Logging\LogMacros.h"


//	 This function must be called once by each module at the beginning, to setup global pointers.
#if defined(_LAUNCHER)
extern "C" void LinkModule(ICore* pCore, const char* moduleName);
#else
extern "C" DLL_EXPORT void LinkModule(ICore* pCore, const char* moduleName);
#endif

// Summary:
//	 Interface of the DLL.
extern "C"
{
	typedef core::traits::Function< ICore*(SCoreInitParams &initParams)> CreateCoreInfterFaceFunc;

	IPCORE_API ICore* CreateCoreInterface(const SCoreInitParams &initParams);

	#define CORE_DLL_NAME "engine_Core.dll"
	#define CORE_DLL_INITFUNC "CreateCoreInterface"

}


#endif // !_X_CORE_I_H_
