#include "stdafx.h"
#include "ScriptCmd.h"
#include "EngineApp.h"

#define _LAUNCHER
#include <ModuleExports.h>

#include <Platform\Console.h>

#include <String\HumanSize.h>
#include <String\HumanDuration.h>

#include <Time\StopWatch.h>

#include <Util\UniquePointer.h>

#include <ICompression.h>
#include <IFileSys.h>

#ifdef X_LIB

struct XRegFactoryNode* g_pHeadToRegFactories = nullptr;

X_LINK_LIB("engine_Core")

#endif // !X_LIB

namespace
{

	typedef core::MemoryArena<
		core::MallocFreeAllocator,
		core::SingleThreadPolicy,
#if X_ENABLE_MEMORY_DEBUG_POLICIES
		core::SimpleBoundsChecking,
		core::SimpleMemoryTracking,
		core::SimpleMemoryTagging
#else
		core::NoBoundsChecking,
		core::NoMemoryTracking,
		core::NoMemoryTagging
#endif // !X_ENABLE_MEMORY_SIMPLE_TRACKING
	> ScriptCmdArena;

	X_DECLARE_ENUM(ScriptMode)(
		CHECK,
		RUN,
		BAKE
	);

	bool GetMode(ScriptMode::Enum& mode)
	{
		const wchar_t* pMode = gEnv->pCore->GetCommandLineArgForVarW(L"mode");
		if (pMode)
		{
			if (core::strUtil::IsEqualCaseInsen(pMode, L"check"))
			{
				mode = ScriptMode::CHECK;
			}
			else if (core::strUtil::IsEqualCaseInsen(pMode, L"run"))
			{
				mode = ScriptMode::RUN;
			}
			else if (core::strUtil::IsEqualCaseInsen(pMode, L"bake"))
			{
				mode = ScriptMode::BAKE;
			}
			else
			{
				X_ERROR("Converter", "Unknown mode: \"%ls\"", pMode);
				return false;
			}

			return true;
		}

		return false;
	}

	bool GetInputFile(core::string& inputFile)
	{
		const wchar_t* pInputFile = gEnv->pCore->GetCommandLineArgForVarW(L"if");
		if (pInputFile)
		{
			core::StackString512 narrowName(pInputFile);

			inputFile = narrowName.c_str();
			return true;
		}

		X_ERROR("Converter", "missing input file");
		return false;
	}



} // namespace 

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	core::Console Console(X_WIDEN(X_ENGINE_NAME) L" - ScriptCmd");
	Console.RedirectSTD();
	Console.SetSize(100, 40, 2000);
	Console.MoveTo(10, 10);

	core::MallocFreeAllocator allocator;
	ScriptCmdArena arena(&allocator, "ScriptCmdArena");

	int res = -1;
	
	{
		EngineApp app;

		if (app.Init(hInstance, &arena, lpCmdLine, Console))
		{
			script::IScriptSys* pScriptSys = gEnv->pScriptSys;

			core::string inputFile;
			ScriptMode::Enum mode;

			if (!GetMode(mode)) {
				mode = ScriptMode::RUN;
			}

			if (!GetInputFile(inputFile)) {
				return -1;
			}

			if (mode == ScriptMode::RUN)
			{


			}
			else if (mode == ScriptMode::CHECK)
			{
				X_ASSERT_NOT_IMPLEMENTED();

			}
			else if (mode == ScriptMode::BAKE)
			{
				X_ASSERT_NOT_IMPLEMENTED();

			}
		}
	}

	return res;
}
