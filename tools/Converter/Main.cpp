#include "stdafx.h"
#include "Converter.h"
#include "EngineApp.h"

#include <Memory\MemoryTrackingPolicies\NoMemoryTracking.h>
#include <Memory\ThreadPolicies\MultiThreadPolicy.h>

#include <Platform\Console.h>
#include <Time\StopWatch.h>

#define _LAUNCHER
#include <ModuleExports.h>

#include <IAnimation.h>

#include "Converter.h"

HINSTANCE g_hInstance = 0;

#ifdef X_LIB

struct XRegFactoryNode* g_pHeadToRegFactories = 0;

X_LINK_LIB("engine_Core")
X_LINK_LIB("engine_RenderNull")

X_FORCE_SYMBOL_LINK("?factory__@XFactory@XEngineModule_Render@@0V12@A")

X_LINK_LIB("engine_ModelLib")
X_LINK_LIB("engine_AnimLib")

X_FORCE_SYMBOL_LINK("?factory__@XFactory@XConverterLib_Model@@0V12@A")
X_FORCE_SYMBOL_LINK("?factory__@XFactory@XConverterLib_Anim@@0V12@A")


#endif // !X_LIB


typedef core::MemoryArena<
	core::MallocFreeAllocator,
	core::SingleThreadPolicy,
	core::SimpleBoundsChecking,
	core::NoMemoryTracking,
	core::SimpleMemoryTagging
> ConverterArena;

core::MemoryArenaBase* g_arena = nullptr;



namespace
{
	X_DECLARE_ENUM(ConvertMode)(SINGLE, ALL);


	bool GetMode(ConvertMode::Enum& mode)
	{
		const wchar_t* pMode = gEnv->pCore->GetCommandLineArgForVarW(L"mode");
		if (pMode)
		{
			if (core::strUtil::IsEqualCaseInsen(pMode, L"single"))
			{
				mode = ConvertMode::SINGLE;
			}
			else if (core::strUtil::IsEqualCaseInsen(pMode, L"all"))
			{
				mode = ConvertMode::ALL;
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

	bool GetAssetType(converter::AssetType::Enum& assType)
	{
		const wchar_t* pAssetType = gEnv->pCore->GetCommandLineArgForVarW(L"type");
		if (pAssetType)
		{
			if (core::strUtil::IsEqualCaseInsen(pAssetType, L"model"))
			{
				assType = converter::AssetType::MODEL;
			}
			else if (core::strUtil::IsEqualCaseInsen(pAssetType, L"anim"))
			{
				assType = converter::AssetType::ANIM;
			}
			else if (core::strUtil::IsEqualCaseInsen(pAssetType, L"material"))
			{
				assType = converter::AssetType::MATERIAL;
			}
			else if (core::strUtil::IsEqualCaseInsen(pAssetType, L"img"))
			{
				assType = converter::AssetType::IMG;
			}
			else
			{		
				X_ERROR("Converter", "Unknown asset type: \"%ls\"", pAssetType);
				return false;
			}

			return true;
		}
	
		X_ERROR("Converter", "missing asset type");
		return false;
	}

	bool GetAssetName(core::string& name)
	{
		const wchar_t* pAssetName = gEnv->pCore->GetCommandLineArgForVarW(L"name");
		if (pAssetName)
		{
			char buf[512];	
			name = core::strUtil::Convert(pAssetName, buf);;
			return true;
		}

		X_ERROR("Converter", "missing asset name");
		return false;
	}

}// namespace 


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	g_hInstance = hInstance;


	core::Console Console(L"Potato - Converter");
	Console.RedirectSTD();
	Console.SetSize(60, 40, 2000);
	Console.MoveTo(10, 10);


	core::MallocFreeAllocator allocator;
	ConverterArena arena(&allocator, "ConverterArena");
	g_arena = &arena;

	bool res = false;

	{
		EngineApp app; // needs to clear up before arena.

		if (app.Init(lpCmdLine, Console))
		{
			converter::Converter con;
			converter::AssetType::Enum assType;
			ConvertMode::Enum mode;
			core::string assName;

			con.PrintBanner();

			if (!GetMode(mode)) {
				mode = ConvertMode::SINGLE;
			}

			core::StopWatch timer;

			if (mode == ConvertMode::ALL)
			{
				if (GetAssetType(assType)) {
					if (!con.ConvertAll(assType)) {
						X_ERROR("Convert", "Conversion failed..");
					}
					else {
						res = true;
					}
				}
				else
				{
					if (!con.ConvertAll()) {
						X_ERROR("Convert", "Conversion failed..");
					}
					else {
						res = true;
					}
				}
			}
			else if (GetAssetType(assType) && GetAssetName(assName)) 
			{
				if (!con.Convert(assType, assName)) {
					X_ERROR("Convert", "Conversion failed..");
				}
				else {
					res = true;
				}
			}

			X_LOG0("Convert", "Elapsed time: ^6%gms", timer.GetMilliSeconds());

			Console.PressToContinue();
		}
	}

	return res ? 0 : -1;
}

