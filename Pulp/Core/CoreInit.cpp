#include "stdafx.h"
#include "Console.h"
#include "Core.h"
#include "Log.h"

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

#include <Extension\IPotatoUnknown.h>
#include <Extension\IPotatoFactory.h>
#include <Extension\PotatoCreateClass.h>

#include <Memory\MemInfo.h>
#include <Platform\MessageBox.h>


#include "PotatoFactoryRegistryImpl.h"

// Logging
#include "Platform\Console.h"

// window
#include "Platform\Window.h"

#include "xFileSys.h"

#if defined(WIN32)

#include <VersionHelpers.h>

#endif // !defined(WIN32)


X_USING_NAMESPACE;

#define DLL_INPUT			"Engine_Input"
#define DLL_FONT			"Engine_Font"
#define DLL_SOUND			"Engine_Sound"
#define DLL_SCRIPT			"Engine_Script"
#define DLL_RENDER			"Engine_RenderDx12"
#define DLL_RENDER_CLASS	"Engine_RenderDx12"
#define DLL_RENDER_NULL		"Engine_RenderNull"
#define DLL_3D_ENGINE		"Engine_3DEngine"
#define DLL_GAME_DLL		"Engine_GameDLL"
#define DLL_PHYSICS			"Engine_Physics"
#define DLL_NET				"Engine_Network"



#if defined(WIN32)
#define DLL_MODULE_INIT_ICORE	"LinkModule"
#define DLL_INITFUNC_INPUT		"CreateInput"
#else
#error "wut.. is this below?"
#define DLL_MODULE_INIT_ICORE (LPCSTR)2
#define DLL_INITFUNC_INPUT     (LPCSTR)1
#endif


namespace
{
	typedef core::traits::Function<void*(ICore*, const char*)> ModuleLinkfunc;

} // namespace

//////////////////////////////////////////////////////////////////////////
#if !defined(X_LIB)

core::Module::Handle XCore::LoadDLL(const char* pDllName)
{
	X_PROFILE_NO_HISTORY_BEGIN("LoadDll", core::profiler::SubSys::CORE);

	core::Module::Handle handle = core::Module::Load(pDllName);
	if (!handle)
	{
		return 0;
	}

	ModuleLinkfunc::Pointer pfnModuleInitICore = reinterpret_cast<ModuleLinkfunc::Pointer>(
		core::Module::GetProc(handle, DLL_MODULE_INIT_ICORE));

	if (pfnModuleInitICore)
	{
		pfnModuleInitICore(this, pDllName);
	}

	return handle;
}
#endif //#if !defined(X_LIB) 


bool XCore::IntializeLoadedEngineModule(const char* pDllName, const char* pModuleClassName)
{
	X_PROFILE_NO_HISTORY_BEGIN("EngModuleInit", core::profiler::SubSys::CORE);

#if !defined(X_LIB)
	core::Module::Handle handle = core::Module::Load(pDllName);

	ModuleLinkfunc::Pointer pfnModuleInitICore = reinterpret_cast<ModuleLinkfunc::Pointer>(
		core::Module::GetProc(handle, DLL_MODULE_INIT_ICORE));

	if (pfnModuleInitICore) {
		pfnModuleInitICore(this, pDllName);
	}

#endif

	std::shared_ptr<IEngineModule> pModule;
	if (PotatoCreateClassInstance(pModuleClassName, pModule))
	{
		if (!pModule->Initialize(env_, initParams_)) {
			X_ERROR("Core", "failed to initialize IEng: %s -> %s", pDllName, pModuleClassName);
			return false;
		}
	}
	else
	{
		X_ERROR("Core", "failed to find interface: %s -> %s", pDllName, pModuleClassName);
		return false;
	}

	// auto clean up by core shutdown.
	moduleInterfaces_.append(pModule);

	return true;
}


bool XCore::IntializeLoadedConverterModule(const char* pDllName, const char* pModuleClassName, 
	IConverterModule** pConvertModuleOut, IConverter** pConverterInstance)
{
	X_PROFILE_NO_HISTORY_BEGIN("ConModuleInit", core::profiler::SubSys::CORE);

	for (auto& c : converterInterfaces_)
	{
		if (c.dllName == pDllName && c.moduleClassName == pModuleClassName)
		{
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

	if (PotatoCreateClassInstance(pModuleClassName, pModule))
	{
		pConverter = pModule->Initialize();
		if (!pConverter) {
			X_ERROR("Core", "failed to initialize ICon: %s -> %s", pDllName, pModuleClassName);
			return false;
		}
	}
	else
	{
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
	for (size_t i= 0; i< converterInterfaces_.size(); i++)
	{
		auto& c = converterInterfaces_[i];
		if (c.pConModule.get() == pConvertModule)
		{
			if (c.removeReference() == 0) 
			{
				if(!c.pConModule->ShutDown(c.pConverter))
				{
					X_ERROR("Core", "error shuting down converter module");
				}

				converterInterfaces_.removeIndex(i);
			}

			return true;
		}
	}
	return false;
}



bool XCore::IntializeEngineModule(const char *dllName, const char *moduleClassName, const SCoreInitParams &initParams)
{
	X_PROFILE_NO_HISTORY_BEGIN("ModuleInit", core::profiler::SubSys::CORE);

	core::Path<char> path(dllName);
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
	if (PotatoCreateClassInstance(moduleClassName, pModule))
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
	int64 memUsed = static_cast<int64_t>(memEnd.WorkingSetSize) - static_cast<int64_t>(memStart.WorkingSetSize);
	core::HumanSize::Str sizeStr;
	X_LOG0("Engine", "ModuleInit \"%s\", MemUsage=%s ^6%gms", name ? (name+1) : dllName, core::HumanSize::toString(sizeStr, memUsed), time.GetMilliSeconds());
#else
	X_LOG0("Engine", "ModuleInit \"%s\": %s ^6%gms", name ? (name + 1) : dllName, res ? "OK" : "Fail", time.GetMilliSeconds());
#endif // #if !defined(X_LIB) 

	return res;
}


//////////////////////////////////////////////////////////////////////////
bool XCore::Init(const SCoreInitParams &startupParams)
{
	core::StopWatch time;

	// init the system baby!
	gEnv->mainThreadId = core::Thread::GetCurrentID();			

	core::invalidParameterHandler::Startup(); 
	core::exceptionHandler::Startup();

	if (startupParams.bProfileSysEnabled)
	{
#if X_ENABLE_PROFILER
		pProfiler_ = X_NEW(core::profiler::XProfileSys, g_coreArena, "ProfileSys")(g_coreArena);
		
		env_.pProfiler = pProfiler_;
#endif // !X_ENABLE_PROFILER
	}


	X_PROFILE_NO_HISTORY_BEGIN("CoreInit", core::profiler::SubSys::CORE);

	if (startupParams.loadSymbols()) {
		core::symbolResolution::Startup();
	}
	initParams_ = startupParams;

#if defined(WIN32)
	{
		bool bIsWindowsXPorLater = ::IsWindowsXPOrGreater();

		if (!bIsWindowsXPorLater)
		{
			core::msgbox::show(L"Versions of windows older than and including XP are not supported.",
				X_WIDEN(X_ENGINE_NAME) L" Start Error",
				core::msgbox::Style::Error | core::msgbox::Style::Topmost | core::msgbox::Style::DefaultDesktop,
				core::msgbox::Buttons::OK);
			return false;
		}
	}
#endif

	if (!ParseCmdArgs(startupParams.pCmdLine)) {
		return false;
	}

	if (!parseSeed(startupParams.seed)) {
		return false;
	}

	// #------------------------- Logging -----------------------
	if (!InitLogging(startupParams)) {
		return false;
	}

	// #------------------------- JOB SYSTEM ------------------------
	if (startupParams.jobSystemEnabled()) {
		env_.pJobSys = X_NEW(core::V2::JobSystem, g_coreArena, "JobSystem");
	}

	// #------------------------- FileSystem --------------------
	if (!InitFileSys(startupParams)) {
		return false;
	}

	// #------------------------- Create Console ----------------
	if (!startupParams.isCoreOnly() || startupParams.basicConsole())
	{
		env_.pConsole = X_NEW(core::XConsole, g_coreArena, "ConsoleSys");
		// register the commands so they can be used before Console::Init
		env_.pConsole->registerVars();
		env_.pConsole->registerCmds();

		// register system vars to the console.
		CreateSystemVars();
	}
	else
	{
		// use a null console saves me adding a branch
		// for every var register command.
		// and i'm still able to detect var registers before core is init. :)
		env_.pConsole = X_NEW( core::XConsoleNULL, g_coreArena, "NullConsole");
	}

	if (!env_.pConsole->init(this, startupParams.basicConsole()))
	{
		return false;
	}

#if X_ENABLE_PROFILER
	// reg profiler vars.
	if (pProfiler_)	{
		pProfiler_->registerVars();
		pProfiler_->registerCmds();
	}
#endif // !X_ENABLE_PROFILER

	// register verbosity vars.
	if (pConsoleLogger_) {
		pConsoleLogger_->GetFilterPolicy().RegisterVars();
	}

	dirWatcher_.Init();

	// register filesystemvars.
	env_.pFileSys->registerVars();
	env_.pFileSys->registerCmds();

	// regeister window vars.
	core::xWindow::RegisterVars();

	// Load the default config.
	if (!startupParams.isCoreOnly()) 
	{
		X_PROFILE_NO_HISTORY_BEGIN("ConfigLoad", core::profiler::SubSys::CORE);

		if (!env_.pConsole->LoadAndExecConfigFile("default.cfg")) {
			return false;
		}

		if (!env_.pConsole->LoadAndExecConfigFile("user_config.cfg")) {
			// this is not required.
		}
	}

	// #------------------------- CPU Info ----------------------
	pCpuInfo_ = X_NEW(core::CpuInfo, g_coreArena, "CpuInfo");
	// #------------------------- Crc32 ----------------------
	pCrc32_ = X_NEW( core::Crc32, g_coreArena, "Crc32");

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
		env_.pJobSys->StartUp();
	}

	// #------------------------- FileSystem Workets ------------------------
	if (startupParams.jobSystemEnabled()) {
		env_.pFileSys->initWorker();
	}

	if(!startupParams.isCoreOnly())
	{
		// #------------------------- Input ------------------------
		if (!InitInput(startupParams)) {
			return false;
		}

		// now we can register the console input listner
		if (!env_.pConsole->registerInputListener()) {
			return false;
		}

		// #------------------------- Font -------------------------
		if (!InitFont(startupParams)) {
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

		// #------------------------- ScriptSys -------------------------
		if (!InitScriptSys(startupParams)) {
			return false;
		}
	}

#if X_ENABLE_PROFILER
	if (pProfiler_) {
		if (!pProfiler_->init(this)) {
			return false;
		}
	}
#endif // !X_ENABLE_PROFILER

	if (startupParams.bEnableNetowrking)
	{
		// #------------------------- Networking -------------------------
		if (!InitNet(startupParams)) {
			return false;
		}
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
			vars_.win_x_pos, vars_.win_y_pos,
			vars_.win_width, vars_.win_height, core::xWindow::Mode::APPLICATION))
		{
			return false;
		}

	//	pWindow_->CustomFrame(true);
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
		if (!InitRenderSys(startupParams)) {
			return false;
		}

#if X_ENABLE_PROFILER
		if(pProfiler_) {
			if (!pProfiler_->loadRenderResources()) {
				return false;
			}
		}
#endif // !X_ENABLE_PROFILER
	}

	// sync net inits before 3d engine.
	if (env_.pNet)
	{
		if (!env_.pNet->asyncInitFinalize())
		{
			return false;
		}
	}

	//  #------------------------- 3DEngine -------------------------
	if (!startupParams.bTesting && !startupParams.isCoreOnly()) {
		if (!Init3DEngine(startupParams)) {
			return false;
		}
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

	AddIgnoredHotReloadExtensions();

	if (startupParams.loadSymbols()) {
		core::symbolResolution::Refresh();
	}


	// #------------------------ Async Init --------------------------
	{
		// Sync up any asynchronous initialization so we are fully init.
		// This is stuff like loading default assets, that can be done in background to 
		// minamize startup time.
		if (!InitAsyncWait())
		{
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

	// we should call all even if one fails.
	// aka we must wait for all to finish even if some fail.
	bool allOk = true;

	allOk &= env_.pConsole->asyncInitFinalize();


	// wait for default font to fully load.
	if (env_.pFontSys)
	{
		font::IFont* pFont = env_.pFontSys->GetFont("default");
		if (!pFont) 
		{
			allOk = false;
			X_ERROR("Font", "failed to get default font");
		}
		else
		{
			if (!pFont->WaitTillReady()) {
				allOk = false;
				X_ERROR("Font", "Error loading default font");
			}
		}
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

	for (i = 0; i < num; i++)
	{
		if (args_.size() >= args_.capacity()) {
			X_WARNING("Core", "Ignoring %" PRIuS " remaning command args, max reached: %" PRIuS, num - args_.size(), args_.capacity());
			break;
		}

		const wchar_t* pArg = args.getArgv(i);
		if ((*pArg == L'+' || *pArg == L'-') && core::strUtil::strlen(pArg) > 1)
		{
			args_.AddOne().AppendArg(args.getArgv(i) + 1);
		}
		else
		{
			if (args_.isEmpty()) {
				args_.AddOne();
			}
			args_[args_.size() - 1].AppendArg(args.getArgv(i));
		}
	}

	return true;
}

bool XCore::parseSeed(const Vec4i& seed)
{
	if (seed != Vec4i::zero()) {
		core::random::MultiplyWithCarrySeed(seed);
		core::random::XorShiftSeed(seed);
	}

	return true;
}


bool XCore::InitFileSys(const SCoreInitParams &initParams)
{
	X_UNUSED(initParams);
	env_.pFileSys = X_NEW_ALIGNED( core::xFileSys, g_coreArena, "FileSys", 8);

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
bool XCore::InitLogging(const SCoreInitParams &initParams)
{
	env_.pLog = X_NEW_ALIGNED( core::XLog, g_coreArena, "LogSystem", 8);

#if X_ENABLE_LOGGING
	if (env_.pLog) 
	{
		if (initParams.bVsLog)
		{
			pVsLogger_ = X_NEW( VisualStudioLogger, g_coreArena, "VSLogger");
			env_.pLog->AddLogger(pVsLogger_);
		}
		if (initParams.bConsoleLog)
		{
			if (initParams.pConsoleWnd) {
				pConsole_ = initParams.pConsoleWnd;
			}
			else {
				pConsole_ = X_NEW( core::Console, g_coreArena, "ExternalConsoleLog")(L"Engine Log");
				pConsole_->SetSize(120, 60, 8000);
				pConsole_->MoveTo(10, 10);
			}

		//	if (!initParams.bTesting) {
			pConsoleLogger_ = X_NEW( ConsoleLogger, g_coreArena,"ConsoleLogger")(
					ConsoleLogger::FilterPolicy(2, "console"),
					ConsoleLogger::FormatPolicy(),
					ConsoleLogger::WritePolicy(*pConsole_));

				env_.pLog->AddLogger(pConsoleLogger_);
		//	}
		}
	}

#else
	X_UNUSED(initParams);
#endif
	return env_.pLog != nullptr;
}

bool XCore::InitConsole(const SCoreInitParams &initParams)
{
	// console can load it's render resources now.
	if (!initParams.basicConsole())
	{
		if (!env_.pConsole->loadRenderResources()) {
			return false;
		}
	}

	return true;
}


bool XCore::InitInput(const SCoreInitParams &initParams)
{
	if (initParams.bSkipInput) {
		env_.pInput = X_NEW(input::XNullInput, gEnv->pArena,"NullInput");
		return true;
	}

	if (!IntializeEngineModule(DLL_INPUT, "Engine_Input", initParams)) {
		return false;
	}

	X_ASSERT_NOT_NULL(env_.pInput);

	env_.pInput->registerVars();
	env_.pInput->registerCmds();

	if (!env_.pInput->Init()) {
		X_ERROR("Font", "failed to init input system");
		return false;
	}

	return true;
}

bool XCore::InitFont(const SCoreInitParams &initParams)
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
 
	if (gEnv->IsDedicated()) { // don't need a font for dedicated.
		return true;
	}

	// load a default font.
	font::IFont* pFont = env_.pFontSys->NewFont("default");
	if (!pFont) {
		X_ERROR("Font", "failed to create default font");
		return false;
	}

	if(!pFont->loadFont(true)) {
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

bool XCore::InitPhysics(const SCoreInitParams& initParams)
{
	if (!IntializeEngineModule(DLL_PHYSICS, "Engine_Physics", initParams)) {
		return false;
	}

	X_ASSERT_NOT_NULL(env_.pPhysics);

	env_.pPhysics->registerVars();
	env_.pPhysics->registerCmds();

	return true;
}


bool XCore::InitNet(const SCoreInitParams& initParams)
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

bool XCore::InitScriptSys(const SCoreInitParams& initParams)
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

bool XCore::InitRenderSys(const SCoreInitParams& initParams)
{
	if (initParams.bTesting)
	{
		if (!IntializeEngineModule(DLL_RENDER_NULL, "Engine_RenderNull", initParams)) {
			return false;
		}
	}
	else
	{
		if (!IntializeEngineModule(DLL_RENDER, DLL_RENDER_CLASS, initParams)) {
			return false;
		}
	}

	if (!env_.pRender) {
		return false;
	}

	const bool reverseZ = render::DEPTH_REVERSE_Z;

	if (initParams.bTesting)
	{
		// should never fail since it's null render system.
		// but it may get changed later that it can fail so check.
		if (!env_.pRender->init(NULL, 0, 0, texture::Texturefmt::UNKNOWN, reverseZ)) {
			X_ERROR("Core", "Failed to init render system");
			return false;
		}
	}
	else
	{
		if (!pWindow_) {
			return false;
		}

		core::StopWatch timer;

		HWND hWnd = pWindow_->GetNativeWindow();
		uint32_t width = pWindow_->GetClientWidth();
		uint32_t height = pWindow_->GetClientHeight();

		// vars required pre init.
		env_.pRender->registerVars();

		if (!env_.pRender->init(hWnd, width, height, texture::Texturefmt::D32_FLOAT, reverseZ)) {
			X_ERROR("Core", "Failed to init render system");
			return false;
		}

		env_.pRender->registerCmds();

		X_LOG0("Core", "render init: ^6%gms", timer.GetMilliSeconds());
	}

	return true;
}


bool XCore::Init3DEngine(const SCoreInitParams& initParams)
{
	if (!IntializeEngineModule(DLL_3D_ENGINE, "Engine_3DEngine", initParams)) {
		return false;
	}

	X_ASSERT_NOT_NULL(env_.p3DEngine);

	core::StopWatch timer;

	if (!env_.p3DEngine->init()) {
		X_ERROR("Core", "Failed to init 3DEngine");
		return false;
	}

	env_.p3DEngine->registerVars();
	env_.p3DEngine->registerCmds();

	X_LOG0("Core", "3DEngine init: ^6%gms", timer.GetMilliSeconds());

	return true;
}

bool XCore::InitGameDll(const SCoreInitParams& initParams)
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
void XCore::WindowPosVarChange(core::ICVar* pVar)
{
	X_UNUSED(pVar);

	int x_pos = vars_.win_x_pos;
	int y_pos = vars_.win_y_pos;

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



void XCore::CreateSystemVars(void)
{
	using namespace core;

	X_ASSERT_NOT_NULL(gEnv);
	X_ASSERT_NOT_NULL(gEnv->pConsole);

	core::xWindow::Rect desktop = core::xWindow::GetDesktopRect();

	ADD_CVAR_REF("core_fast_shutdown", vars_.core_fast_shutdown, 0, 0, 1, VarFlag::SYSTEM | VarFlag::SAVE_IF_CHANGED,
		"Skips most cleanup logic for faster shutdown, when off everything is correctly shutdown and released before exit. 0=off 1=on");

	ADD_CVAR_REF("core_event_debug", vars_.core_event_debug, 0, 0, 1, VarFlag::SYSTEM,
		"Debug messages for core events. 0=off 1=on");

	var_win_pos_x = ADD_CVAR_REF("win_x_pos", vars_.win_x_pos, 10, 0, desktop.getWidth(),
		VarFlag::SYSTEM | VarFlag::SAVE_IF_CHANGED, "Game window position x");
	var_win_pos_y = ADD_CVAR_REF("win_y_pos", vars_.win_y_pos, 10, 0, desktop.getHeight(),
		VarFlag::SYSTEM | VarFlag::SAVE_IF_CHANGED, "Game window position y");
	var_win_width = ADD_CVAR_REF("win_width", vars_.win_width, 800, 800, 1,
		VarFlag::SYSTEM , "Game window width");
	var_win_height = ADD_CVAR_REF("win_height", vars_.win_height, 600, 600, 1,
		VarFlag::SYSTEM , "Game window height");

	core::ConsoleVarFunc del;
	del.Bind<XCore, &XCore::WindowPosVarChange>(this);
	var_win_pos_x->SetOnChangeCallback(del);

	del.Bind<XCore, &XCore::WindowPosVarChange>(this);
	var_win_pos_y->SetOnChangeCallback(del);

	del.Bind<XCore, &XCore::WindowSizeVarChange>(this);
	var_win_width->SetOnChangeCallback(del);

	del.Bind<XCore, &XCore::WindowSizeVarChange>(this);
	var_win_height->SetOnChangeCallback(del);

	var_win_custom_Frame = ADD_CVAR_INT("win_custom_Frame", 1, 0, 1,
		VarFlag::SYSTEM | VarFlag::SAVE_IF_CHANGED, "Enable / disable the windows custom frame");

	del.Bind<XCore, &XCore::WindowCustomFrameVarChange>(this);
	var_win_custom_Frame->SetOnChangeCallback(del);


	var_profile = ADD_CVAR_INT("profile", 1, 0, 1, VarFlag::SYSTEM, "Enable Profiling");


	ADD_CVAR_STRING("version", X_ENGINE_NAME "Engine " X_BUILD_STRING " Version " X_ENGINE_VERSION_STR, VarFlag::SYSTEM | VarFlag::READONLY, "Engine Version");
	ADD_CVAR_STRING("build_ref", X_STRINGIZE(X_ENGINE_BUILD_REF), VarFlag::SYSTEM | VarFlag::READONLY, "Engine Version");

	ADD_COMMAND_MEMBER("filesysHotReloadEtxList", this, XCore, &XCore::Command_HotReloadListExts, VarFlag::SYSTEM,
		"Display all registered file extensions in the hotreload system");
	
	ADD_COMMAND_MEMBER("listProgramArgs", this, XCore, &XCore::Command_ListProgramArgs, VarFlag::SYSTEM,
		"Lists the processed command line arguments parsed to the program");

}

void XCore::AddIgnoredHotReloadExtensions(void)
{
#if X_DEBUG

	// don't warn about shit like .txt!
	hotReloadIgnores_.append(core::string("txt"));

#endif // !X_DEBUG
}

// ->->->->->->->->->->->->->->->->->->->->->->->->->->->->->->->->->->->


void XCore::RegisterAssertHandler(IAssertHandler* errorHandler)
{
	X_ASSERT(!env_.isRunning(), "Assert handlers must be registerd at statup")(env_.isRunning(), errorHandler);

	assertHandlers_.push_back(errorHandler);
}

void XCore::UnRegisterAssertHandler(IAssertHandler* errorHandler)
{
	assertHandlers_.remove(errorHandler);
}

void XCore::OnAssert(const core::SourceInfo& sourceInfo)
{
	for (auto* pHandler : assertHandlers_)
	{
		pHandler->OnAssert(sourceInfo);
	}
}

void XCore::OnAssertVariable(const core::SourceInfo& sourceInfo)
{
	for (auto* pHandler : assertHandlers_)
	{
		pHandler->OnAssertVariable(sourceInfo);
	}
}

