#include "stdafx.h"
#include "Core.h"
#include "Console.h"
#include "ConsoleNULL.h"
#include "Log.h"

#include <String\StackString.h>

#include "NullImplementation/NullInput.h"

#include <Debugging\InvalidParameterHandler.h>
#include <Debugging\SymbolResolution.h>

#include <IInput.h>
#include <IEngineModule.h>
#include <IScriptSys.h>
#include <IRender.h>
#include <I3DEngine.h>
#include <IFont.h>
#include <IFileSys.h>
#include <IGame.h>
#include <IJobSystem.h>

#include <Extension\IGoatClass.h>
#include <Extension\IGoatFactory.h>
#include <Extension\GoatCreateClass.h>

#include <Memory\MemInfo.h>

#include "GoatFactoryRegistryImpl.h"

// Logging
#include "Platform\Console.h"

// window
#include "Platform\Window.h"

#include "xFileSys.h"


X_USING_NAMESPACE;

#define DLL_INPUT			"Engine_Input"
#define DLL_FONT			"Engine_Font"
#define DLL_SOUND			"Engine_Sound"
#define DLL_SCRIPT			"Engine_Script"
#define DLL_RENDER			"Engine_RenderDx10"
#define DLL_RENDER_NULL		"Engine_RenderNull"
#define DLL_3D_ENGINE		"Engine_3DEngine"
#define DLL_GAME_DLL		"Engine_GameDLL"



#if defined(WIN32)
#define DLL_MODULE_INIT_ICORE	"LinkModule"
#define DLL_INITFUNC_INPUT		"CreateInput"
#else
#define DLL_MODULE_INIT_ICORE (LPCSTR)2
#define DLL_INITFUNC_INPUT     (LPCSTR)1
#endif


//////////////////////////////////////////////////////////////////////////
#if !defined(X_LIB)
WIN_HMODULE XCore::LoadDynamiclibrary(const char *dllName) const
{
	WIN_HMODULE handle = NULL;

	handle = GoatLoadLibary(dllName);

	return handle;
}


WIN_HMODULE XCore::LoadDLL(const char *dllName)
{
	WIN_HMODULE handle = LoadDynamiclibrary(dllName);

	if (!handle)
	{
		return 0;
	}

	// std::string moduleName = dllName; // PathUtil::GetFileName(dllName);

	typedef void *(*PtrFunc_LinkModule)(ICore *pSystem, const char *moduleName);
	PtrFunc_LinkModule pfnModuleInitISystem = (PtrFunc_LinkModule)GoatGetProcAddress(handle, DLL_MODULE_INIT_ICORE);
	if (pfnModuleInitISystem)
	{
		pfnModuleInitISystem(this, dllName);
	}

	return handle;
}
#endif //#if !defined(X_LIB) 


bool XCore::IntializeEngineModule(const char *dllName, const char *moduleClassName, const SCoreInitParams &initParams)
{
	core::Path<char> path(dllName);
	
	path.setExtension(".dll");

#if !defined(X_LIB)
	core::XProcessMemInfo memStart, memEnd;
	core::GetProcessMemInfo(memStart);
#endif // #if !defined(X_LIB)

#if !defined(X_LIB) 
	WIN_HMODULE hModule = LoadDLL(path.c_str());
	if (!hModule)
		return false;

	moduleDLLHandles_.push_back(hModule);
#endif // #if !defined(X_LIB) 

	bool res = false;


	std::shared_ptr<IEngineModule> pModule;
	if (GoatCreateClassInstance(moduleClassName, pModule))
	{
		res = pModule->Initialize(env_, initParams);
	}
	else
	{
		X_ERROR("Core", "failed to find interface: %s -> %s", dllName, moduleClassName);
		return false;
	}

	moduleInterfaces_.append(pModule);

	const char* name = core::strUtil::Find(dllName, dllName + strlen(dllName), "_");

#if !defined(X_LIB)
	core::GetProcessMemInfo(memEnd);
	uint64 memUsed = memEnd.WorkingSetSize - memStart.WorkingSetSize;
	X_LOG0("Engine", "Init \"%s\", MemUsage=%dKb", name ? (name+1) : dllName, uint32(memUsed / 1024));
#else
	X_LOG0("Engine", "Init \"%s\": %s", name ? (name + 1) : dllName, res ? "OK" : "Fail");
#endif // #if !defined(X_LIB) 

	return res;
}


//////////////////////////////////////////////////////////////////////////
bool XCore::Init(const SCoreInitParams &startupParams)
{
	// init the system baby!
	gEnv->uMainThreadId = GetCurrentThreadId();			//Set this ASAP on startup

	core::invalidParameterHandler::Startup(); 
	core::symbolResolution::Startup();

	initParams_ = startupParams;

#if defined(WIN32)
	X_DISABLE_WARNING(4996);
	{
		OSVERSIONINFOW osvi;

		core::zero_object(osvi);

		osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOW);
		GetVersionExW(&osvi);
		
		bool bIsWindowsXPorLater = osvi.dwMajorVersion > 5 || (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion >= 1);

		if (!bIsWindowsXPorLater)
		{
			::MessageBoxW(reinterpret_cast<HWND>(startupParams.hWnd), 
				L"Versions of windows older than XP are not supported.",
				L"Critial Error", MB_OK);
			return false;
		}
	}
	X_ENABLE_WARNING(4996);
#endif

	hInst_ = (WIN_HINSTANCE)startupParams.hInstance;
	hWnd_ = (WIN_HWND)startupParams.hWnd;

	// #------------------------- Logging -----------------------
	if (!InitLogging(startupParams))
		return false;

	// #------------------------- FileSystem --------------------
	if (!InitFileSys(startupParams))
		return false;

	// #------------------------- Create Console ----------------
	if (!startupParams.isCoreOnly())
	{
		env_.pConsole = X_NEW_ALIGNED(core::XConsole, g_coreArena, "ConsoleSys",8);

		// register system vars to the console.
		CreateSystemVars();
	}
	else
	{
		// use a null console saves me adding a branch
		// for every var register command.
		// and i'm still able to detect var registers before core is init. :)
		env_.pConsole = X_NEW( core::XConsoleNULL, g_coreArena, "NullConsole");
		env_.pConsole->ShutDown();
	}

	// register filesystemvars.
	env_.pFileSys->CreateVars();

	// regeister window vars.
	core::xWindow::RegisterVars();

	// Load the default config.
	if (!env_.pConsole->LoadConfig("default.cfg")) {
		return false;
	}

	// #------------------------- CPU Info ----------------------
	pCpuInfo_ = X_NEW( core::CpuInfo, g_coreArena, "CpuInfo");

	// Call init on objects from before so they can register vars.
	env_.pLog->Init();


	// #------------------------- TIMER ------------------------
	if (!time_.Init(this))
		return false;

	// #------------------------- JOB SYSTEM ------------------------
	if (env_.pJobSys) {
		env_.pJobSys->StartUp();
	}

	// #------------------------- ProfileSys ---------------------------
	profileSys_.Init(this);


	if(!startupParams.isCoreOnly())
	{
		// #------------------------- Input ------------------------
		if (!InitInput(startupParams))
			return false;

		// #------------------------- Font -------------------------
		if (!InitFont(startupParams))
			return false;

		// #------------------------- Sound -------------------------
		if (!InitSound(startupParams))
			return false;

		// #------------------------- ScriptSys -------------------------
		if (!InitScriptSys(startupParams))
			return false;
	}


	if (!startupParams.bTesting && !startupParams.isCoreOnly())
	{
		// create a window
		pWindow_ = X_NEW(core::xWindow, g_coreArena, "GameWindow");

		wchar_t titleW[128];
		const char* pTitle = X_ENGINE_NAME " Engine " X_CPUSTRING
#if X_SUPER == 0
			" (fps:0, 0ms) Time: 0(x1)"
#endif
		;

		if (!pWindow_->Create(core::strUtil::Convert(pTitle, titleW),
			g_coreVars.win_x_pos, g_coreVars.win_y_pos, 
			g_coreVars.win_width, g_coreVars.win_height, core::xWindow::Mode::APPLICATION))
		{
			return false;
		}

		pWindow_->CustomFrame(true);
		pWindow_->HideClientCursor(true);

		// Alogn it
		if (pConsole_)
		{
			pWindow_->AlignTo(core::xWindow::GetPrimaryRect(), 
				Alignment::TOP_ALIGN | Alignment::RIGHT_ALIGN);
		}
	}


	// #------------------------- Renderer -------------------------
	if (!startupParams.isCoreOnly() || startupParams.bTesting) {
		if (!InitRenderSys(startupParams))
			return false;
	}

	//  #------------------------- 3DEngine -------------------------
	if (!startupParams.bTesting && !startupParams.isCoreOnly()) {
		if (!Init3DEngine(startupParams))
			return false;	
	}

	//  #------------------------- Game Dll -------------------------
	if (!startupParams.bTesting && !startupParams.isCoreOnly()) {
		if (!InitGameDll(startupParams))
			return false;
	}

	// #------------------------- Console ---------------------------
	if (!InitConsole())
		return false;


	core::symbolResolution::Refresh();

	// show the window
	if (pWindow_)
		pWindow_->Show();
	return true;
}

bool XCore::InitFileSys(const SCoreInitParams &initParams)
{
	X_UNUSED(initParams);
	env_.pFileSys = X_NEW_ALIGNED( core::xFileSys, g_coreArena, "FileSys", 8);

	if (env_.pFileSys) {
		if (!env_.pFileSys->Init()) {
			X_ERROR("Core", "Failed to init filesystem");
			return false;
		}
	}

	return env_.pFileSys != nullptr;
}


//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
bool XCore::InitLogging(const SCoreInitParams &initParams)
{
	env_.pLog = X_NEW_ALIGNED( core::XLog, g_coreArena, "LogSystem", 8);

	if (env_.pLog) {
		if (initParams.bVsLog)
		{
			pVsLogger_ = X_NEW( VisualStudioLogger, g_coreArena, "VSLogger");
			env_.pLog->AddLogger(pVsLogger_);
		}
		if (initParams.bConsoleLog)
		{
			if (initParams.pConsoleWnd)
				pConsole_ = initParams.pConsoleWnd;
			else {
				pConsole_ = X_NEW( core::Console, g_coreArena, "ExternalConsoleLog")(L"Engine Log");
				pConsole_->SetSize(120, 60, 8000);
				pConsole_->MoveTo(10, 10);
			}

		//	if (!initParams.bTesting) {
			pConsoleLogger_ = X_NEW( ConsoleLogger, g_coreArena,"ConsoleLogger")(
					ConsoleLogger::FilterPolicy(),
					ConsoleLogger::FormatPolicy(),
					ConsoleLogger::WritePolicy(*pConsole_));

				env_.pLog->AddLogger(pConsoleLogger_);
		//	}
		}
	}

	return env_.pLog != nullptr;
}

bool XCore::InitConsole()
{
	env_.pConsole->Startup(this);

	return true;
}


bool XCore::InitInput(const SCoreInitParams &initParams)
{
	if (initParams.bSkipInput) {
		env_.pInput = X_NEW(input::XNullInput, gEnv->pArena,"NullInput");
		return true;
	}

	if (!IntializeEngineModule(DLL_INPUT, "Engine_Input", initParams))
		return false;

	return env_.pInput != nullptr;
}

bool XCore::InitFont(const SCoreInitParams &initParams)
{
	if (!IntializeEngineModule(DLL_FONT, "Engine_Font", initParams))
		return false;

	if (!env_.pFont)
		return false;

	if (gEnv->IsDedicated()) // don't need a font for dedicated.
		return true;

	// load a default font.
	font::IFFont* font = env_.pFont->NewFont("default");

	if (font == nullptr) {
		X_ERROR("Font", "failed to create default font");
		return false;
	}

	if(!font->loadFont())
	{
		X_ERROR("Font", "failed to load default font");
		return false;
	}

	return true;
}

bool XCore::InitSound(const SCoreInitParams& initParams)
{
	if (initParams.bSkipSound) {
		env_.pSound = nullptr;
		return true;
	}

	if (!IntializeEngineModule(DLL_SOUND, "Engine_Sound", initParams))
		return false;

	return env_.pSound != nullptr;
}

bool XCore::InitScriptSys(const SCoreInitParams& initParams)
{
	if (!IntializeEngineModule(DLL_SCRIPT, "Engine_Script", initParams))
		return false;

	if (!env_.pScriptSys)
		return false;

	env_.pScriptSys->Init();

	return true;
}

bool XCore::InitRenderSys(const SCoreInitParams& initParams)
{
	if (initParams.bTesting)
	{
		if (!IntializeEngineModule(DLL_RENDER_NULL, "Engine_RenderNull", initParams))
			return false;
	}
	else
	{
		if (!IntializeEngineModule(DLL_RENDER, "Engine_RenderDx10", initParams))
			return false;
	}

	if (!env_.pRender)
		return false;

	if (initParams.bTesting)
	{
		env_.pRender->Init(NULL, 0, 0);

	}
	else
	{
		if (!pWindow_) {
			return false;
		}

		HWND hWnd = pWindow_->GetNativeWindow();
		uint32_t width = pWindow_->GetClientWidth();
		uint32_t height = pWindow_->GetClientHeight();

		if (!env_.pRender->Init(hWnd, width, height)) {
			X_ERROR("Core", "Failed to init render system");
			return false;
		}
	}

	return true;
}


bool XCore::Init3DEngine(const SCoreInitParams& initParams)
{
	if (!IntializeEngineModule(DLL_3D_ENGINE, "Engine_3DEngine", initParams))
		return false;

	if (env_.p3DEngine) {
		if (!env_.p3DEngine->Init()) {
			X_ERROR("Core", "Failed to init 3DENninge");
			return false;
		}
	}

	return env_.p3DEngine != nullptr;
}

bool XCore::InitGameDll(const SCoreInitParams& initParams)
{
	if (!IntializeEngineModule(DLL_GAME_DLL, "Engine_Game", initParams))
		return false;

	if (env_.pGame) {
		if (!env_.pGame->Init()) {
			X_ERROR("Core", "Failed to init Game");
			return false;
		}
	}

	return env_.pGame != nullptr;
}


// ->->->->->->->->->->->->->->->->->->->->->->->->->->->->->->->->->->->
void WindowPosVarChange(core::ICVar* pVar)
{
	X_UNUSED(pVar);

	int x_pos = g_coreVars.win_x_pos;
	int y_pos = g_coreVars.win_y_pos;

	core::xWindow* win = gEnv->pCore->GetGameWindow();

	win->MoveTo(x_pos, y_pos);
}

void WindowSizeVarChange(core::ICVar* pVar)
{
	X_UNUSED(pVar);

}


void XCore::CreateSystemVars()
{
	using namespace core;

	X_ASSERT_NOT_NULL(gEnv);
	X_ASSERT_NOT_NULL(gEnv->pConsole);

	core::xWindow::Rect desktop = core::xWindow::GetDesktopRect();


	ADD_CVAR_REF("core_event_debug", g_coreVars.core_event_debug, 0, 0, 1, VarFlag::SYSTEM,
		"Debug messages for core events. 0=off 1=on");

	var_win_pos_x = ADD_CVAR_REF("win_x_pos", g_coreVars.win_x_pos, 10, 0, desktop.getWidth(), 
		VarFlag::SYSTEM | VarFlag::SAVE_IF_CHANGED, "Game window position x");
	var_win_pos_y = ADD_CVAR_REF("win_y_pos", g_coreVars.win_y_pos, 10, 0, desktop.getHeight(), 
		VarFlag::SYSTEM | VarFlag::SAVE_IF_CHANGED, "Game window position y");
	var_win_width = ADD_CVAR_REF("win_width", g_coreVars.win_width, 800, 800, 1, 
		VarFlag::SYSTEM , "Game window width");
	var_win_height = ADD_CVAR_REF("win_height", g_coreVars.win_height, 600, 600, 1, 
		VarFlag::SYSTEM , "Game window height");

	var_win_pos_x->SetOnChangeCallback(WindowPosVarChange);
	var_win_pos_y->SetOnChangeCallback(WindowPosVarChange);
	var_win_width->SetOnChangeCallback(WindowSizeVarChange);
	var_win_height->SetOnChangeCallback(WindowSizeVarChange);


	var_profile = ADD_CVAR_INT("profile", 1, 0, 1, VarFlag::SYSTEM, "Enable Profiling");
	var_profile_draw = ADD_CVAR_INT("profile_draw", 0, 0, 1, VarFlag::SYSTEM, "Display profiler");


	ADD_CVAR_STRING("version", X_ENGINE_NAME "Engine " X_BUILD_STRING " Version " X_ENGINE_VERSION_STR, VarFlag::SYSTEM | VarFlag::STATIC, "Engine Version");

	ADD_COMMAND("filesys_hotreload_etx_list", Command_HotReloadListExts, VarFlag::SYSTEM,
		"Display all registered file extensions in the hotreload system");
	

	dirWatcher_.Init();
}

// ->->->->->->->->->->->->->->->->->->->->->->->->->->->->->->->->->->->


void XCore::RegisterAssertHandler(IAssertHandler* errorHandler)
{
	assertHandlers_.push_back(errorHandler);
}

void XCore::UnRegisterAssertHandler(IAssertHandler* errorHandler)
{
	assertHandlers_.removeIndex(assertHandlers_.find(errorHandler));
}

void XCore::OnAssert(const core::SourceInfo& sourceInfo)
{
	core::Array<IAssertHandler*>::ConstIterator end = assertHandlers_.end();
	for (core::Array<IAssertHandler*>::ConstIterator it = assertHandlers_.begin(); it != end; ++it)
	{
		(*it)->OnAssert(sourceInfo);
	}
}

void XCore::OnAssertVariable(const core::SourceInfo& sourceInfo)
{
	core::Array<IAssertHandler*>::ConstIterator end = assertHandlers_.end();
	for (core::Array<IAssertHandler*>::ConstIterator it = assertHandlers_.begin(); it != end; ++it)
	{
		(*it)->OnAssertVariable(sourceInfo);
	}
}

