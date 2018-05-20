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

#ifdef X_LIB
struct XRegFactoryNode* g_pHeadToRegFactories = nullptr;

// X_LINK_ENGINE_LIB("Font")
X_LINK_ENGINE_LIB("Core")
X_LINK_ENGINE_LIB("RenderNull")
X_LINK_ENGINE_LIB("Physics");

X_FORCE_LINK_FACTORY("XConverterLib_Material")
X_FORCE_LINK_FACTORY("XConverterLib_Model")
X_FORCE_LINK_FACTORY("XConverterLib_Phys")

#endif // !X_LIB

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPTSTR lpCmdLine,
    _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);

    // compile my anus.
    int res = -1;

    {
        core::MallocFreeAllocator allocator;
        LvlBuilderArena arena(&allocator, "LevelBuilderArena");

        level::g_arena = &arena;

        {
            EngineApp engine;

            // we need the engine for Assets, Logging, Profiling, FileSystem.
            if (engine.Init(hInstance, lpCmdLine, &arena)) {
                {
                    core::ICVar* pLogVerbosity = gEnv->pConsole->GetCVar("log_verbosity");
                    X_ASSERT_NOT_NULL(pLogVerbosity);
                    pLogVerbosity->Set(0);
                }

                core::Path<char> path;
                path.set("map_source\\");
                path.setFileName("basic - Copy.map");
                path.setFileName("alcatraz.map");
                path.setFileName("killzone.map");
                path.setFileName("box.map");
                path.setFileName("boxmap.map");
                path.setFileName("box2.map");
                path.setFileName("box3.map");
                path.setFileName("box4.map");
                path.setFileName("boxmap.map");
                path.setFileName("portal_test.map");
                path.setFileName("entity_test.map");
                path.setFileName("physics_test.map");
                path.setFileName("test01.map");

                level::Compiler comp(&arena, engine.GetPhysCooking());

                if (comp.init()) {
                    core::Path<char> outPath;
                    outPath.setFileName(path.fileName());

                    if (comp.compileLevel(path, outPath)) {
                        res = 0;
                    }
                }

                X_LOG0("Level", "Operation Complete...");
            }

            level::g_arena = nullptr;
        }
    }

    return res;
}
