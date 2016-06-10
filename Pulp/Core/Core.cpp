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
#include <IEngineModule.h>

#include <Hashing\crc32.h>
#include <Platform\Window.h>
#include <Platform\Console.h>
#include <Platform\SystemInfo.h>
#include <Debugging\InvalidParameterHandler.h>
#include <Debugging\CallStack.h>
#include <Memory\VirtualMem.h>
#include <String\HumanSize.h>

#include <Console.h>

#include "CoreEventDispatcher.h"

#include <Threading\JobSystem2.h>

#if X_PLATFORM_WIN32
#include <conio.h>
#endif // !X_PLATFORM_WIN32

SCoreGlobals XCore::env_;
core::MallocFreeAllocator XCore::malloc_;

XCoreVars g_coreVars;


XCore::XCore() :
	pWindow_(nullptr),
	pConsole_(nullptr),
	pCpuInfo_(nullptr),
	pVsLogger_(nullptr),
	pConsoleLogger_(nullptr),
	pEventDispatcher_(nullptr),
//	arena_(&malloc_, "GlobalMalloc"),
	
	moduleDLLHandles_(g_coreArena),
	moduleInterfaces_(g_coreArena),
	assertHandlers_(g_coreArena),
	dirWatcher_(g_coreArena),
	hotReloadExtMap_(g_coreArena),

#if X_DEBUG
	hotReloadIgnores_(g_coreArena),
#endif // !X_DEBUG

	strAlloc_(1 << 24, core::VirtualMem::GetPageSize() * 2, 
	StrArena::getMemoryAlignmentRequirement(8), 
	StrArena::getMemoryOffsetRequirement() + 12),

	numArgs_(0),

	var_win_pos_x(nullptr),
	var_win_pos_y(nullptr),
	var_win_width(nullptr),
	var_win_height(nullptr),
	var_win_custom_Frame(nullptr),

	var_profile(nullptr)
{
	X_ASSERT_NOT_NULL(g_coreArena);

	memset(&env_, 0, sizeof(env_));

	hotReloadExtMap_.reserve(32);

	pEventDispatcher_ = X_NEW( core::XCoreEventDispatcher, g_coreArena, "CoreEventDispatch");
	pEventDispatcher_->RegisterListener(this);

	env_.pCore = this;
	env_.pTimer = &time_;
	env_.pDirWatcher = &dirWatcher_;
	env_.pHotReload = this;
//	env_.pMalloc = &g_coreArena;
	env_.pArena = g_coreArena;
	env_.pStrArena = X_NEW(StrArena, g_coreArena, "StrArena")(&strAlloc_,"StrArens");
	env_.pArena->addChildArena(env_.pStrArena);
	// vars.

	env_.client_ = true;
	env_.dedicated_ = false;
	env_.profilerEnabled_ = false;

	// created in coreInit.
	//	env_.pJobSys = X_NEW(core::JobSystem, g_coreArena, "JobSystem");
	//env_.pJobSys = X_NEW(core::V2::JobSystem, g_coreArena, "JobSystem");

	dirWatcher_.registerListener(this);
}


XCore::~XCore()
{
	ShutDown();


	env_.pCore = nullptr;
	env_.pTimer = nullptr;
	env_.pDirWatcher = nullptr;
	env_.pHotReload = nullptr;
	env_.pArena = nullptr;
}

void XCore::Release()
{

	X_DELETE(this, g_coreArena);

}


void XCore::ShutDown()
{
	X_LOG0("Core", "Shutting Down");

#if X_DEBUG
	hotReloadIgnores_.free();
#endif // !X_DEBUG

	dirWatcher_.ShutDown();

	if (env_.pJobSys)
	{
		env_.pJobSys->ShutDown();
		core::Mem::DeleteAndNull(env_.pJobSys, g_coreArena);
	}

	if (env_.pGame)
	{
		env_.pGame->ShutDown();
		core::SafeRelease(env_.pGame);
	}

	if (env_.pConsole)
	{
		env_.pConsole->freeRenderResources();
	}

	if (env_.pRender) {
	//	env_.pRender->ShutDown();
	}

	if (env_.pPhysics)
	{
		env_.pPhysics->ShutDown();
		core::SafeRelease(env_.pPhysics);
	}

	if (env_.pSound)
	{
		env_.pSound->ShutDown();
		core::SafeRelease(env_.pSound);
	}

	if (env_.pFont)
	{
		env_.pFont->ShutDown();
		core::SafeRelease(env_.pFont);
	}

	if (env_.pScriptSys)
	{
		env_.pScriptSys->ShutDown();
		core::SafeRelease(env_.pScriptSys);
	}

	if (pWindow_)
	{
		pWindow_->Destroy();
		core::Mem::DeleteAndNull(pWindow_, g_coreArena);
	}

	if (pCpuInfo_)
	{
		core::Mem::DeleteAndNull(pCpuInfo_, g_coreArena);
	}

	if (env_.p3DEngine)
	{
		env_.p3DEngine->ShutDown();
		core::SafeRelease(env_.p3DEngine);
	}

	if (env_.pConsole && !initParams_.basicConsole())
	{
		env_.pConsole->SaveChangedVars();
	}

	if (env_.pFileSys)
	{
		env_.pFileSys->ShutDown();
		core::Mem::DeleteAndNull(env_.pFileSys, g_coreArena);

	}


	// free any listners here.
	if (env_.pConsole)
	{
		env_.pConsole->unregisterInputListener();
	}

	// needs to be done after engine, since it has input listners.
	if (env_.pInput)
	{
		env_.pInput->ShutDown();
		core::SafeRelease(env_.pInput);
	}


	if (env_.pRender)
	{
		env_.pRender->ShutDown();
		// We don't delete it's static in DLL.
	//	DeleteAndNull(env_.pRender);
	}


	// shut down interfaces before logging is removed.
	for (auto it : moduleInterfaces_) {
		it->ShutDown();
	}

	moduleInterfaces_.free();

#if X_PLATFORM_WIN32 // && X_DEBUG
	if (!initParams_.bTesting) { // don't pause when testing.
		_getch();
	}
#endif // !X_PLATFORM_WIN32

	if (env_.pConsole)
	{
		env_.pConsole->ShutDown();
		core::Mem::DeleteAndNull(env_.pConsole, g_coreArena);
	}

	if (env_.pStrArena)
	{
		core::Mem::DeleteAndNull(env_.pStrArena, g_coreArena);
	}


	// Loggers last.
	if (env_.pLog)
	{
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
		if (initParams_.pConsoleWnd == nullptr)
			X_DELETE(pConsole_, g_coreArena);

		core::Mem::DeleteAndNull(env_.pLog, g_coreArena);
	}

	if (pEventDispatcher_) {
		pEventDispatcher_->RemoveListener(this);
		core::Mem::DeleteAndNull(pEventDispatcher_, g_coreArena);
	}


	for (size_t i = 0; i < moduleDLLHandles_.size(); i++) {
		PotatoFreeLibrary(moduleDLLHandles_[i]);
	}

	moduleDLLHandles_.free();


//	core::invalidParameterHandler::Shutdown();
}


core::Crc32* XCore::GetCrc32()
{
	static core::Crc32 crcGen;
	return &crcGen;
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

void XCore::OnCoreEvent(CoreEvent::Enum event, UINT_PTR wparam, UINT_PTR lparam)
{
//	X_UNUSED(event);
	X_UNUSED(wparam);
	X_UNUSED(lparam);


	switch (event)
	{
		case CoreEvent::MOVE:
		{
			core::xWindow::Rect rect = pWindow_->GetRect();

			g_coreVars.win_x_pos = rect.getX1();
			g_coreVars.win_y_pos = rect.getY1();

			if (var_win_pos_x) {
				var_win_pos_x->SetModified();
			}
			if (var_win_pos_x) {
				var_win_pos_y->SetModified();
			}
		}
		break;
		case CoreEvent::RESIZE:
		{
			core::xWindow::Rect rect = pWindow_->GetClientRect();

			g_coreVars.win_height = rect.getHeight();
			g_coreVars.win_width = rect.getWidth();

			if (var_win_width) {
				var_win_width->SetModified();
			}
			if (var_win_height) {
				var_win_height->SetModified();
			}
		}
		break;
		case CoreEvent::ACTIVATE:
		{
			if (pWindow_) {
				pWindow_->ClipCursorToWindow();
			}
		}
		break;
	}
}



const wchar_t* XCore::GetCommandLineArgForVarW(const wchar_t* pVarName)
{
	X_ASSERT_NOT_NULL(pVarName);

	if (numArgs_ == 0) {
		return nullptr;
	}

	size_t i;
	for (i = 0; i < numArgs_; i++)
	{
		const CmdArg& arg = args_[i];

		if (arg.getArgc() >= 3)
		{
			const wchar_t* pCmd = arg.getArgv(0);
			if (core::strUtil::IsEqualCaseInsen(pCmd, L"set"))
			{
				const wchar_t* pName = arg.getArgv(1);
				if (core::strUtil::IsEqual(pName, pVarName))
				{
					const wchar_t* pValue = arg.getArgv(2);
					return pValue;
				}
			}
		}
		else if (arg.getArgc() == 2)
		{
			const wchar_t* pName = arg.getArgv(0);
			if (core::strUtil::IsEqualCaseInsen(pName, pVarName))
			{
				const wchar_t* pValue = arg.getArgv(1);
				return pValue;
			}
		}
	}

	return nullptr;
}



// IXHotReloadManager
bool XCore::addfileType(core::IXHotReload* pHotReload, const char* extension)
{
//	X_ASSERT_NOT_NULL(pHotReload);
	X_ASSERT_NOT_NULL(extension);
	
	if (core::strUtil::Find(extension, '.')) {
		X_ERROR("HotReload", "extension can't contain dots");
		return false;
	}

	// note: hotReloadExtMap_ stores char* pointer.
	if (pHotReload == nullptr) {
		hotReloadExtMap_.erase(extension);
		return true;
	}

	if (hotReloadExtMap_.find(extension) != hotReloadExtMap_.end()) {
		X_ERROR("HotReload", "failed to register file type, it already has a handler");
		return false;
	}

	hotReloadExtMap_.insert(hotReloadMap::value_type(extension, pHotReload));
	return true;
}

// ~IXHotReloadManager



void XCore::OnFatalError(const char* format, va_list args)
{
	core::CallStack::Description Dsc;
	core::CallStack stack(1);
	
	stack.ToDescription(Dsc);
	
	X_LOG0("FatalError", "CallStack:\n%s", Dsc);


	core::LoggerBase::Line Line;
	vsnprintf_s(Line, sizeof(core::LoggerBase::Line), _TRUNCATE, format, args);

	::MessageBoxA(NULL, Line, X_ENGINE_NAME" Error", MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);


	X_BREAKPOINT;

	_exit(1);
}


void XCore::Command_HotReloadListExts(core::IConsoleCmdArgs* Cmd)
{
	X_UNUSED(Cmd);

	HotReloadListExts();
}

void XCore::Command_ListProgramArgs(core::IConsoleCmdArgs* Cmd)
{
	X_UNUSED(Cmd);

	ListProgramArgs();
}

void XCore::HotReloadListExts(void)
{
	X_LOG0("HotReload", "------- ^8Registerd Extensions^7 -------");

	hotReloadMap::const_iterator it;

	it = hotReloadExtMap_.begin();

	for (; it != hotReloadExtMap_.end(); ++it)
	{
		X_LOG0("HotReload", "^1%s", it->first);
	}

	X_LOG0("HotReload", "----- ^8Registerd Extensions End^7 -----");
}

void XCore::ListProgramArgs(void)
{
	size_t i, num = numArgs_;

	X_LOG0("AppArgs", "------ ^8Program Args(%" PRIuS ")^7 ------", num);

	core::StackString<1024 + 128> merged;

	for (i = 0; i < num; i++)
	{
		const auto& arg = args_[i];
		size_t j, argsNum = arg.getArgc();

		merged.appendFmt("(%i) ", i);

		for (j = 0; j < argsNum; j++)
		{
			merged.appendFmt("^1%ls^7", arg.getArgv(j));
			if ((j+1) < argsNum) {
				merged.append(" -> ");
			}
		}

		X_LOG0("AppArgs", "%s", merged.c_str());
	}

	X_LOG0("AppArgs", "------ ^8Program Args End^7 -----");
}


void XCore::LogSystemInfo(void) const
{
	core::SysInfo::UserNameStr userName;
	core::SysInfo::LanguageStr lang;
	core::SysInfo::MemInfo memInfo;
	core::SysInfo::DisplayInfo displayInfo;

	core::SysInfo::GetUserName(userName);
	core::SysInfo::GetLanguage(lang);
	core::SysInfo::GetSystemMemInfo(memInfo);
	core::SysInfo::GetDisplayInfo(displayInfo);

	core::HumanSize::Str s1, s2, s3;

	X_LOG0("SysInfo", "UserName: \"%ls\"", userName);
	X_LOG0("SysInfo", "Language: \"%ls\"", lang);
	X_LOG0("SysInfo", "PhysicalMem %s available %s virtual %s used %ld%%",
		core::HumanSize::toString(s1, memInfo.TotalPhys),
		core::HumanSize::toString(s2, memInfo.AvailPhys),
		core::HumanSize::toString(s3, memInfo.TotalVirtual),
		memInfo.dwMemoryLoad
	);
	X_LOG0("SysInfo", "Display: %dx%dx%d",
		displayInfo.pelsWidth,
		displayInfo.pelsHeight,
		displayInfo.bitsPerPel
	);

}

X_NAMESPACE_BEGIN(core)




X_NAMESPACE_END