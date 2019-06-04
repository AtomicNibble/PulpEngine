#include "stdafx.h"
#include "EngineApp.h"

#include <Memory\MemoryTrackingPolicies\NoMemoryTracking.h>
#include <Memory\ThreadPolicies\MultiThreadPolicy.h>

#include <Platform\Console.h>
#include <Time\StopWatch.h>
#include <Hashing\Fnva1Hash.h>
#include <String\HumanDuration.h>

#define _LAUNCHER
#include <ModuleExports.h>

#include <IAnimation.h>

#include <../ConverterLib/ConverterLib.h>
X_LINK_ENGINE_LIB("ConverterLib")
X_LINK_ENGINE_LIB("assetDB")

#ifdef X_LIB

struct XRegFactoryNode* g_pHeadToRegFactories = nullptr;

X_LINK_ENGINE_LIB("Core")
X_LINK_ENGINE_LIB("RenderNull")

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
    >
    ConverterArena;

using namespace core::string_view_literals;
using namespace core::Hash::Literals;

namespace
{
    X_DECLARE_ENUM(ConvertMode)
    (SINGLE, ALL, CLEAN, CLEAN_THUMBS, GEN_THUMBS, CHKDSK, CLEAN_RAW_FILE, REPACK, DUMP);

    bool GetMode(ConvertMode::Enum& mode)
    {
        auto modeStr = gEnv->pCore->GetCommandLineArg("mode"_sv);
        if (modeStr.isNotEmpty()) {

            core::StackString256 modeStr(modeStr.begin(), modeStr.end());
            modeStr.toLower();

            switch (core::Hash::Fnv1aHash(modeStr.data(), modeStr.length()))
            {
                case "single"_fnv1a:
                    mode = ConvertMode::SINGLE;
                    break;
                case "all"_fnv1a:
                    mode = ConvertMode::ALL;
                    break;
                case "clean"_fnv1a:
                    mode = ConvertMode::CLEAN;
                    break;
                case "clean_thumbs"_fnv1a:
                    mode = ConvertMode::CLEAN_THUMBS;
                    break;
                case "gen_thumbs"_fnv1a:
                    mode = ConvertMode::GEN_THUMBS;
                    break;
                case "chkdsk"_fnv1a:
                    mode = ConvertMode::CHKDSK;
                    break;
                case "clean_raw_file"_fnv1a:
                    mode = ConvertMode::CLEAN_RAW_FILE;
                    break;
                case "repack"_fnv1a:
                    mode = ConvertMode::REPACK;
                    break;
                case "dump"_fnv1a:
                    mode = ConvertMode::DUMP;
                    break;
                default:
                    X_ERROR("Converter", "Unknown mode: \"%.*s\"", modeStr.length(), modeStr.data());
                    return false;
            }
        
            return true;
        }

        return false;
    }

    bool GetAssetType(converter::AssetType::Enum& assType, bool slient = false)
    {
        auto assetTypeStr = gEnv->pCore->GetCommandLineArg("type"_sv);
        if (assetTypeStr.isNotEmpty()) {
            if (!assetTypeFromStr(assetTypeStr.begin(), assetTypeStr.end(), assType)) {
                X_ERROR("Converter", "Unknown asset type: \"%.*s\"", assetTypeStr.length(), assetTypeStr.data());
                return false;
            }

            return true;
        }

        X_ERROR_IF(!slient, "Converter", "missing asset type");
        return false;
    }

    bool GetAssetName(core::string& name, bool slient = false)
    {
        auto assetNameStr = gEnv->pCore->GetCommandLineArg("name"_sv);
        if (assetNameStr.isNotEmpty()) {
            name.assign(assetNameStr.begin(), assetNameStr.end());
            return true;
        }

        X_ERROR_IF(!slient, "Converter", "missing asset name");
        return false;
    }

    bool GetConversionProfile(core::string& name, bool slient = false)
    {
        auto profileNameStr = gEnv->pCore->GetCommandLineArg("profile"_sv);
        if (profileNameStr.isNotEmpty()) {
            name.assign(profileNameStr.begin(), profileNameStr.end());
            return true;
        }

        return false;
    }

    bool ForceModeEnabled(void)
    {
        auto forceStr = gEnv->pCore->GetCommandLineArg("force"_sv);
        if (forceStr.isNotEmpty()) {
            return core::strUtil::StringToBool(forceStr.begin(), forceStr.end());
        }
        return false;
    }

    void GetMod(core::string& name)
    {
        auto modNameStr = gEnv->pCore->GetCommandLineArg("mod"_sv);
        if (modNameStr.empty()) {
            return;
        }

        name.assign(modNameStr.begin(), modNameStr.end());
    }


    bool run(core::MemoryArenaBase* arena)
    {
        assetDb::AssetDB db;

        converter::Converter con(db, arena);
        converter::AssetType::Enum assType;
        ConvertMode::Enum mode;
        core::string assName;

        con.PrintBanner();
        con.forceConvert(ForceModeEnabled());

        core::string modName;
        GetMod(modName);

        if (!con.Init()) {
            X_ERROR("Convert", "Failed to init converter");
            return false;
        }

        if (modName.isNotEmpty()) {
            if (!con.SetMod(modName)) {
                X_ERROR("Convert", "Failed to set mod");
            }
        }

        core::string profile;
        if (GetConversionProfile(profile)) {
            con.setConversionProfiles(profile);
        }

        if (!GetMode(mode)) {
            mode = ConvertMode::SINGLE;
        }

        core::StopWatch timer;

        if (mode == ConvertMode::CLEAN) {
            if (!con.CleanAll()) {
                X_ERROR("Convert", "Failed to perform clearAll");
                return false;
            }
        }
        if (mode == ConvertMode::CLEAN_THUMBS) {
            if (!con.CleanThumbs()) {
                X_ERROR("Convert", "Failed to clean thumbs");
                return false;
            }
        }
        else if (mode == ConvertMode::GEN_THUMBS) {
            if (!con.GenerateThumbs()) {
                X_ERROR("Convert", "Failed to generate thumbs");
                return false;
            }
        }
        else if (mode == ConvertMode::CHKDSK) {
            if (!con.Chkdsk()) {
                X_ERROR("Convert", "Failed to perform Chkdsk");
                return false;
            }
        }
        else if (mode == ConvertMode::CLEAN_RAW_FILE) {
            if (!con.CleanupOldRawFiles()) {
                X_ERROR("Convert", "Failed to perform raw file cleanup");
                return false;
            }
        }
        else if (mode == ConvertMode::REPACK) {
            if (!con.Repack()) {
                X_ERROR("Convert", "Failed to perform repack");
                return false;
            }
        }
        else if (mode == ConvertMode::DUMP) {
            core::Path<char> path("asset_db/db.json");
            if (!db.Export(path)) {
                X_ERROR("Convert", "Failed to dump db");
                return false;
            }
        }
        else if (mode == ConvertMode::ALL) {
            // optionaly convert all asset of Type X
            if (GetAssetType(assType, true)) {
                if (!con.Convert(assType)) {
                    X_ERROR("Convert", "Conversion failed..");
                    return false;
                }
            }
            else {
                if (!con.ConvertAll()) {
                    X_ERROR("Convert", "Conversion failed..");
                    return false;
                }
            }
        }
        else if (GetAssetType(assType) && GetAssetName(assName)) {
            if (!con.Convert(assType, assName)) {
                X_ERROR("Convert", "Conversion failed..");
                return false;
            }
        }

        core::HumanDuration::Str timeStr;
        X_LOG0("Convert", "Elapsed time: ^6%s", core::HumanDuration::toString(timeStr, timer.GetMilliSeconds()));
        return true;
    }

} // namespace

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nCmdShow)
{
    core::MallocFreeAllocator allocator;
    ConverterArena arena(&allocator, "ConverterArena");

    bool res = false;

    {
        EngineApp app; // needs to clear up before arena.

        if (app.Init(hInstance, &arena, lpCmdLine)) 
        {
            res = run(&arena);

            gEnv->pConsoleWnd->pressToContinue();
        }
    }

    return res ? 0 : 1;
}
