#include "stdafx.h"
#include "EngineApp.h"

#include <Memory\MemoryTrackingPolicies\NoMemoryTracking.h>
#include <Memory\ThreadPolicies\MultiThreadPolicy.h>

#include <Platform\Console.h>
#include <Time\StopWatch.h>
#include <Hashing\Fnva1Hash.h>

#define _LAUNCHER
#include <ModuleExports.h>

#include <IAnimation.h>

#include <../ConverterLib/ConverterLib.h>
X_LINK_LIB("engine_ConverterLib")
X_LINK_LIB("engine_assetDB")


#ifdef X_LIB

struct XRegFactoryNode* g_pHeadToRegFactories = nullptr;

X_LINK_LIB("engine_Core")
X_LINK_LIB("engine_RenderNull")


X_FORCE_SYMBOL_LINK("?s_factory@XEngineModule_Render@render@Potato@@0V?$XSingletonFactory@VXEngineModule_Render@render@Potato@@@@A");
X_FORCE_LINK_FACTORY("XEngineModule_ConverterLib");


#endif // !X_LIB


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
> ConverterArena;


namespace
{
	X_DECLARE_ENUM(ConvertMode)(SINGLE, ALL, CLEAN, GEN_THUMBS, CHKDSK);


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
			else if (core::strUtil::IsEqualCaseInsen(pMode, L"clean"))
			{
				mode = ConvertMode::CLEAN;
			}
			else if (core::strUtil::IsEqualCaseInsen(pMode, L"gen_thumbs"))
			{
				mode = ConvertMode::GEN_THUMBS;
			}
			else if (core::strUtil::IsEqualCaseInsen(pMode, L"chkdsk"))
			{
				mode = ConvertMode::CHKDSK;
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

	bool GetAssetType(converter::AssetType::Enum& assType, bool slient = false)
	{
		using namespace core::Hash::Fnv1Literals;

		const wchar_t* pAssetType = gEnv->pCore->GetCommandLineArgForVarW(L"type");
		if (pAssetType)
		{
			core::StackString<128, char> assetTypeStr(pAssetType);

			switch (core::Hash::Fnv1aHash(assetTypeStr.c_str(), assetTypeStr.length()))
			{
				case "model"_fnv1a:
					assType = converter::AssetType::MODEL;
					break;
				case "anim"_fnv1a:
					assType = converter::AssetType::ANIM;
					break;
				case "material"_fnv1a:
					assType = converter::AssetType::MATERIAL;
					break;
				case "img"_fnv1a:
					assType = converter::AssetType::IMG;
					break;
				case "weapon"_fnv1a:
					assType = converter::AssetType::WEAPON;
					break;

				default:
					X_ERROR("Converter", "Unknown asset type: \"%ls\"", pAssetType);
					return false;
			}

			return true;
		}
	
		X_ERROR_IF(!slient, "Converter", "missing asset type");
		return false;
	}

	bool GetAssetName(core::string& name, bool slient = false)
	{
		const wchar_t* pAssetName = gEnv->pCore->GetCommandLineArgForVarW(L"name");
		if (pAssetName)
		{
			core::StackString512 assetName(pAssetName);

			name = assetName.c_str();
			return true;
		}

		X_ERROR_IF(!slient, "Converter", "missing asset name");
		return false;
	}

	bool GetConversionProfile(core::string& name, bool slient = false)
	{
		const wchar_t* pProfileName = gEnv->pCore->GetCommandLineArgForVarW(L"profile");
		if (pProfileName)
		{
			core::StackString512 profileStr(pProfileName);

			name = profileStr.c_str();
			return true;
		}

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

}// namespace 


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	core::Console Console(X_WIDEN(X_ENGINE_NAME) L" - Converter");
	Console.RedirectSTD();
	Console.SetSize(60, 40, 2000);
	Console.MoveTo(10, 10);


	core::MallocFreeAllocator allocator;
	ConverterArena arena(&allocator, "ConverterArena");

	bool res = false;

	{
		EngineApp app; // needs to clear up before arena.

		if (app.Init(hInstance, &arena, lpCmdLine, Console))
		{
			assetDb::AssetDB db;

			converter::Converter con(db, &arena);
			converter::AssetType::Enum assType;
			ConvertMode::Enum mode;
			core::string assName;

			con.PrintBanner();
			con.forceConvert(ForceModeEnabled());
			
			if (con.Init())
			{
				core::string profile;
				if (GetConversionProfile(profile)) {
					con.setConversionProfiles(profile);
				}

				if (!GetMode(mode)) {
					mode = ConvertMode::SINGLE;
				}

				core::StopWatch timer;

				if (mode == ConvertMode::CLEAN)
				{
					if (!con.CleanAll())
					{
						X_ERROR("Convert", "Failed to perform clearAll");
					}
				}
				else if (mode == ConvertMode::GEN_THUMBS)
				{
					if (!con.GenerateThumbs())
					{
						X_ERROR("Convert", "Failed to generate thumbs");
					}
				}
				else if (mode == ConvertMode::CHKDSK)
				{
					if(!con.Chkdsk())
					{
						X_ERROR("Convert", "Failed to perform Chkdsk");
					}
				}
				else if (mode == ConvertMode::ALL)
				{
					// optionaly convert all asset of Type X
					if (GetAssetType(assType, true)) {
						if (!con.Convert(assType)) {
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
			}
			else
			{
				X_ERROR("Convert", "Failed to init converter");
			}

			Console.PressToContinue();
		}
	}

	return res ? 0 : -1;
}

