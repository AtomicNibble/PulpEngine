#include "stdafx.h"
#include "EngineApp.h"

#include <Memory\MemoryTrackingPolicies\NoMemoryTracking.h>
#include <Memory\ThreadPolicies\MultiThreadPolicy.h>

#include <Platform\Console.h>
#include <Time\StopWatch.h>

#define _LAUNCHER
#include <ModuleExports.h>

#include "TechDefCompiler.h"

#ifdef X_LIB

struct XRegFactoryNode* g_pHeadToRegFactories = nullptr;

X_LINK_ENGINE_LIB("Core")
X_LINK_ENGINE_LIB("RenderNull")

X_FORCE_SYMBOL_LINK("?s_factory@XEngineModule_Render@render@Potato@@0V?$XSingletonFactory@VXEngineModule_Render@render@Potato@@@@A");


X_FORCE_LINK_FACTORY("XConverterLib_Shader");

#endif // !X_LIB

namespace
{

	typedef core::MemoryArena<
		core::MallocFreeAllocator,
		core::MultiThreadPolicy<core::Spinlock>,
#if X_ENABLE_MEMORY_DEBUG_POLICIES
		core::SimpleBoundsChecking,
		core::SimpleMemoryTracking,
		core::SimpleMemoryTagging
#else
		core::NoBoundsChecking,
		core::NoMemoryTracking,
		core::NoMemoryTagging
#endif // !X_ENABLE_MEMORY_SIMPLE_TRACKING
	> CompilerArena;




	X_DECLARE_ENUM(CompileMode)(SINGLE, ALL, CLEAN);


	bool GetMode(CompileMode::Enum& mode)
	{
		const wchar_t* pMode = gEnv->pCore->GetCommandLineArgForVarW(L"mode");
		if (pMode)
		{
			if (core::strUtil::IsEqualCaseInsen(pMode, L"single"))
			{
				mode = CompileMode::SINGLE;
			}
			else if (core::strUtil::IsEqualCaseInsen(pMode, L"all"))
			{
				mode = CompileMode::ALL;
			}
			else if (core::strUtil::IsEqualCaseInsen(pMode, L"clean"))
			{
				mode = CompileMode::CLEAN;
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

	bool GetMaterialCat(engine::MaterialCat::Enum& matCat, bool slient = false)
	{
		const wchar_t* pAssetType = gEnv->pCore->GetCommandLineArgForVarW(L"type");
		if (pAssetType)
		{
			core::StackString256 typeStr(pAssetType);

			matCat = engine::Util::MatCatFromStr(typeStr.c_str());
			if(matCat == engine::MaterialCat::UNKNOWN)
			{
				X_ERROR("Converter", "Unknown material cat: \"%ls\"", pAssetType);
				return false;
			}

			return true;
		}

		X_ERROR_IF(!slient, "Converter", "missing material cat");
		return false;
	}

	bool GetTechName(core::string& name, bool slient = false)
	{
		const wchar_t* pTechName = gEnv->pCore->GetCommandLineArgForVarW(L"name");
		if (pTechName)
		{
			core::StackString512 techNameStr(pTechName);

			name = techNameStr.c_str();
			return true;
		}

		X_ERROR_IF(!slient, "Converter", "missing tech name");
		return false;
	}

	bool ForceModeEnabled(void)
	{
		const wchar_t* pForce = gEnv->pCore->GetCommandLineArgForVarW(L"force");
		if (pForce)
		{
			return core::strUtil::StringToBool(pForce);
		}
		return false;
	}

	bool DebugCompile(void)
	{
		const wchar_t* pDebug = gEnv->pCore->GetCommandLineArgForVarW(L"debug");
		if (pDebug)
		{
			return core::strUtil::StringToBool(pDebug);
		}
		return false;
	}

}// namespace 

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	core::Console Console(X_WIDEN(X_ENGINE_NAME) L" - ShaderCompiler");
	Console.RedirectSTD();
	Console.SetSize(60, 40, 2000);
	Console.MoveTo(10, 10);

	core::MallocFreeAllocator allocator;
	CompilerArena arena(&allocator, "ShaderCompilerArena");

	bool res = false;

	{
		EngineApp app; // needs to clear up before arena.

		if (app.Init(hInstance, &arena, lpCmdLine, Console))
		{
			engine::compiler::TechDefCompiler con(&arena);
			engine::MaterialCat::Enum matCat;
			CompileMode::Enum mode;
			core::string techName;

			con.PrintBanner();

			if (con.Init())
			{
				con.setForceCompile(ForceModeEnabled());
				if (DebugCompile()) {
					con.setCompileFlags(
						render::shader::CompileFlag::Debug |
						render::shader::CompileFlag::OptimizationLvl0 |
						render::shader::CompileFlag::TreatWarningsAsErrors
					);
				}

				if (!GetMode(mode)) {
					mode = CompileMode::SINGLE;
				}

				core::StopWatch timer;

				if (mode == CompileMode::CLEAN)
				{
					if (!con.CleanAll())
					{
						X_ERROR("ShaderCompiler", "Failed to clean..");
					}
				}
				else if (mode == CompileMode::ALL)
				{
					if (GetMaterialCat(matCat, true))
					{
						if (!con.Compile(matCat)) {
							X_ERROR("ShaderCompiler", "Compile failed..");
						}
						else {
							res = true;
						}
					}
					else
					{
						if (!con.CompileAll()) {
							X_ERROR("ShaderCompiler", "Compile failed..");
						}
						else {
							res = true;
						}
					}

				}
				else if (GetMaterialCat(matCat) && GetTechName(techName))
				{
					if (!con.Compile(matCat, techName)) {
						X_ERROR("ShaderCompiler", "Compile failed..");
					}
					else {
						res = true;
					}
				}

				X_LOG0("ShaderCompiler", "Elapsed time: ^6%gms", timer.GetMilliSeconds());

			}
			else
			{
				X_ERROR("ShaderCompiler", "Failed to init compiler");
			}

			Console.PressToContinue();
		}
	}


	return res ? 0 : -1;
}

