#include "stdafx.h"
#include "EngineApp.h"

#include <Memory\MemoryTrackingPolicies\NoMemoryTracking.h>
#include <Memory\ThreadPolicies\MultiThreadPolicy.h>

#include <String\HumanDuration.h>
#include <Platform\Console.h>
#include <Time\StopWatch.h>
#include <Hashing\Fnva1Hash.h>

#define _LAUNCHER
#include <ModuleExports.h>

#include <../LinkerLib/LinkerLib.h>
X_LINK_ENGINE_LIB("LinkerLib")
X_LINK_ENGINE_LIB("assetDB")

#ifdef X_LIB

struct XRegFactoryNode* g_pHeadToRegFactories = nullptr;

X_LINK_ENGINE_LIB("Core")
X_LINK_ENGINE_LIB("RenderNull")

X_FORCE_SYMBOL_LINK("?s_factory@XEngineModule_Render@render@Potato@@0V?$XSingletonFactory@VXEngineModule_Render@render@Potato@@@@A");
X_FORCE_LINK_FACTORY("XEngineModule_LinkerLib");
X_FORCE_LINK_FACTORY("XEngineModule_ConverterLib");

#endif // !X_LIB

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
    >
    LinkerArena;

using namespace core::string_view_literals;

namespace
{
    X_DECLARE_ENUM(LinkMode)
    (BUILD, META);


    void PrintArgs(void)
    {
        X_LOG0("Linker", "Args:");
        X_LOG0("Linker", "^6-mode^7         (build, meta)");
        X_LOG0("Linker", "BuildArgs:");
        X_LOG0("Linker", "^6-al^7           (asset list)");
        X_LOG0("Linker", "^6-lvl^7          (level name)");
        X_LOG0("Linker", "^6-of^7           (output file)");
        X_LOG0("Linker", "^6-mod^7          (mod id/name)");
        X_LOG0("Linker", "^6-nocompress^7   (disable compression)");
        X_LOG0("Linker", "^6-sharedict^7    (use shared compression dict)");
        X_LOG0("Linker", "^6-memory^7       (Hint that pak should be kept in memory)");
        X_LOG0("Linker", "MetaArgs:");
        X_LOG0("Linker", "^6-if^7           (input file)");
    }

    bool GetMode(LinkMode::Enum& mode)
    {
        auto modeStr = gEnv->pCore->GetCommandLineArg("mode"_sv);
        if (modeStr.isNotEmpty()) {
            if (core::strUtil::IsEqualCaseInsen(modeStr, "build"_sv)) {
                mode = LinkMode::BUILD;
            }
            else if (core::strUtil::IsEqualCaseInsen(modeStr, "meta"_sv)) {
                mode = LinkMode::META;
            }
            else {
                X_ERROR("Linker", "Unknown mode: \"%.*s\"", modeStr.length(), modeStr.data());
                return false;
            }

            return true;
        }

        return false;
    }

    bool GetInputfile(core::Path<wchar_t>& name)
    {
        const wchar_t* pFileName = gEnv->pCore->GetCommandLineArgForVarW(L"if");
        if (!pFileName) {
            return false;
        }
         
        name.set(pFileName);
        return true;
    }

    bool LoadBuildOptions(linker::BuildOptions& options)
    {
        if (!gEnv->pCore->GetCommandLineArg("nocompress"_sv).isNotEmpty()) {
            options.flags.Set(AssetPak::PakBuilderFlag::COMPRESSION);
        } 

        if (gEnv->pCore->GetCommandLineArg("sharedict"_sv).isNotEmpty()) {
            options.flags.Set(AssetPak::PakBuilderFlag::SHARED_DICT);
        }

        if (gEnv->pCore->GetCommandLineArg("memory"_sv).isNotEmpty()) {
            options.flags.Set(AssetPak::PakBuilderFlag::HINT_MEMORY);
        }

        if (gEnv->pCore->GetCommandLineArg("timestamp"_sv).isNotEmpty()) {
            options.flags.Set(AssetPak::PakBuilderFlag::TIMESTAMP);
        }

        auto assetListStr = gEnv->pCore->GetCommandLineArg("al"_sv);
        if (assetListStr.isNotEmpty()) {
            options.assetList.set(assetListStr.begin(), assetListStr.end());

            core::Path<char> temp(options.assetList);
            temp.removeExtension();

            options.outFile = temp.fileName();
        }

        auto levelName = gEnv->pCore->GetCommandLineArg("lvl"_sv);
        if (levelName.isNotEmpty()) {
            options.level.assign(levelName.data(), levelName.length());
            options.outFile.set(options.level.begin(), options.level.end());
        }

        auto modName = gEnv->pCore->GetCommandLineArg("mod"_sv);
        if (modName.isNotEmpty()) {
            options.mod.assign(modName.data(), modName.length());
        }

        auto outFileName = gEnv->pCore->GetCommandLineArg("of"_sv);
        if (outFileName.isNotEmpty()) {
            options.outFile.set(outFileName.begin(), outFileName.end());
        }

        if (options.outFile.isEmpty()) {
            X_ERROR("Linker", "Missing output file -of");
            return false;
        }

        return true;
    }


    bool Run(LinkerArena& arena)
    {
        PrintArgs();

        assetDb::AssetDB db;

        linker::Linker linker(db, &arena);
        linker.PrintBanner();

        if (!linker.Init()) {
            X_ERROR("Linker", "Failed to init linker");
            return false;
        }

        LinkMode::Enum mode;
        if (!GetMode(mode)) {
            mode = LinkMode::META;
        }

        core::StopWatch timer;

        if (mode == LinkMode::BUILD) 
        {
            linker::BuildOptions options;

            if (!LoadBuildOptions(options)) {
                X_ERROR("Linker", "Failed to load build options");
                return false;
            }

            if (!linker.Build(options)) {
                X_ERROR("Linker", "Failed to perform build");
                return false;
            }
        }
        else if (mode == LinkMode::META) 
        {
            core::Path<wchar_t> inputFile;

            if (!GetInputfile(inputFile)) {

                int32_t numArgs = __argc;
                if (numArgs == 2) {
                    inputFile.set(__wargv[1]);
                }
                
                if (inputFile.isEmpty()) {
                    X_ERROR("Linker", "Failed to get input file for meta dump");
                    return false;
                }
            }

            if (!linker.dumpMetaOS(inputFile)) {
                X_ERROR("Linker", "Failed to dump meta");
                return false;
            }
        }

        core::HumanDuration::Str timeStr;
        X_LOG0("Linker", "Elapsed time: ^6%s", core::HumanDuration::toString(timeStr, timer.GetMilliSeconds()));
        return true;
    }

} // namespace

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nCmdShow)
{
    core::MallocFreeAllocator allocator;
    LinkerArena arena(&allocator, "LinkerArena");

    bool res = false;

    {
        EngineApp app; // needs to clear up before arena.

        if (app.Init(hInstance, &arena, lpCmdLine)) 
        {
            res = Run(arena);

            gEnv->pConsoleWnd->pressToContinue();
        }
    }

    return res ? 0 : 1;
}
