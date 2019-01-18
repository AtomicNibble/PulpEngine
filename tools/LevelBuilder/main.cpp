#include "stdafx.h"
#include "EngineApp.h"

#include <IConsole.h>
#include "Compiler.h"

#define _LAUNCHER
#include <ModuleExports.h>

#include <Memory\MemoryTrackingPolicies\NoMemoryTracking.h>
#include <Memory\MemoryTrackingPolicies\FullMemoryTracking.h>
#include <Memory\AllocationPolicies\GrowingPoolAllocator.h>
#include <Memory\VirtualMem.h>

#include <../AssetDB/AssetDB.h>

X_NAMESPACE_BEGIN(level)

core::MemoryArenaBase* g_arena = nullptr;

X_NAMESPACE_END

namespace
{
    typedef core::MemoryArena<
        core::MallocFreeAllocator,
        core::SingleThreadPolicy,
#if X_ENABLE_MEMORY_DEBUG_POLICIES
        core::SimpleBoundsChecking,
        core::FullMemoryTracking,
        core::SimpleMemoryTagging
#else
        core::NoBoundsChecking,
        core::NoMemoryTracking,
        core::NoMemoryTagging
#endif // !X_ENABLE_MEMORY_SIMPLE_TRACKING
        >
        LvlBuilderArena;

} // namespace

X_LINK_ENGINE_LIB("MaterialLib");
X_LINK_ENGINE_LIB("ModelLib");
X_LINK_ENGINE_LIB("Physics");
X_LINK_ENGINE_LIB("AssetDB");
X_LINK_ENGINE_LIB("LinkerLib");

#ifdef X_LIB
struct XRegFactoryNode* g_pHeadToRegFactories = nullptr;

// X_LINK_ENGINE_LIB("Font")
X_LINK_ENGINE_LIB("Core")
X_LINK_ENGINE_LIB("RenderNull")
X_LINK_ENGINE_LIB("Physics");

X_FORCE_LINK_FACTORY("XConverterLib_Material")
X_FORCE_LINK_FACTORY("XConverterLib_Model")
X_FORCE_LINK_FACTORY("XConverterLib_Phys")
X_FORCE_LINK_FACTORY("XEngineModule_LinkerLib")

#endif // !X_LIB

namespace
{
    void PrintArgs(void)
    {
        X_LOG0("Level", "Args:");
        X_LOG0("Level", "^6-if^7           (map file)");
        X_LOG0("Level", "^6-mod^7          (set active mod)");
    }

    bool GetInputfile(core::Path<char>& name)
    {
        const wchar_t* pFileName = gEnv->pCore->GetCommandLineArgForVarW(L"if");
        if (!pFileName) {
            return false;
        }

        core::StackString512 nameNarrow(pFileName);
        name = core::Path<char>(nameNarrow.begin(), nameNarrow.end());
        return true;
    }

    void GetMod(core::string& name)
    {
        const wchar_t* pModName = gEnv->pCore->GetCommandLineArgForVarW(L"mod");
        if (!pModName) {
            return;
        }

        core::StackString512 nameNarrow(pModName);
        name.assign(nameNarrow.begin(), nameNarrow.end());
    }


    bool Run(LvlBuilderArena& arena, physics::IPhysicsCooking* pCooking)
    {
        core::Path<char> path;
        if (!GetInputfile(path)) {
            X_ERROR("Level", "Failed to get input file");
            return false;
        }

        path.replaceSeprators();

        core::string modName;
        GetMod(modName);

        assetDb::AssetDB db;

        level::Compiler comp(&arena, db, pCooking);
        if (!comp.init(modName)) {
            return false;
        }

        auto modId = db.GetCurrentModId();

        assetDb::AssetDB::Mod modInfo;
        if (!db.GetModInfo(modId, modInfo)) {
            return false;
        }

        core::Path<char> outPath;
        assetDb::AssetDB::GetOutputPathForAssetType(assetDb::AssetType::LEVEL, modInfo.outDir, outPath);
        outPath.ensureSlash();
        outPath.setFileName(path.fileName());
        outPath.removeExtension();
        outPath.replaceSeprators();

        level::LevelBuilderFlags flags;

        if (!comp.compileLevel(path, outPath, flags)) {
            return false;
        }

        X_LOG0("Level", "Operation Complete...");
        return true;
    }

} // namespace

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPTSTR lpCmdLine,
    _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);

    // compile my anus.
    int res = 1;

    using namespace core::string_view_literals;

    {
        core::MallocFreeAllocator allocator;
        LvlBuilderArena arena(&allocator, "LevelBuilderArena");

        level::g_arena = &arena;

        {
            EngineApp engine;

            // we need the engine for Assets, Logging, Profiling, FileSystem.
            if (engine.Init(hInstance, lpCmdLine, &arena)) 
            {
                {
                    core::ICVar* pLogVerbosity = gEnv->pConsole->getCVar("log_verbosity"_sv);
                    X_ASSERT_NOT_NULL(pLogVerbosity);
                    pLogVerbosity->Set(0);
                }

                if (Run(arena, engine.GetPhysCooking())) {
                    res = 0;
                }
            }

            level::g_arena = nullptr;
        }
    }

    return res;
}
