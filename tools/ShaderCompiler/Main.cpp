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

using namespace core::string_view_literals;

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
        >
        CompilerArena;

    X_DECLARE_ENUM(CompileMode)
    (SINGLE, ALL, CLEAN);

    bool GetMode(CompileMode::Enum& mode)
    {
        auto modeStr = gEnv->pCore->GetCommandLineArg("mode"_sv);
        if (modeStr) {
            if (core::strUtil::IsEqualCaseInsen(modeStr, "single"_sv)) {
                mode = CompileMode::SINGLE;
            }
            else if (core::strUtil::IsEqualCaseInsen(modeStr, "all"_sv)) {
                mode = CompileMode::ALL;
            }
            else if (core::strUtil::IsEqualCaseInsen(modeStr, "clean"_sv)) {
                mode = CompileMode::CLEAN;
            }
            else {
                X_ERROR("Converter", "Unknown mode: \"%.*s\"", modeStr.length(), modeStr.data());
                return false;
            }

            return true;
        }

        return false;
    }

    bool GetMaterialCat(engine::MaterialCat::Enum& matCat, bool slient = false)
    {
        auto typeStr = gEnv->pCore->GetCommandLineArg("type"_sv);
        if (typeStr) {
            matCat = engine::Util::MatCatFromStr(typeStr.begin(), typeStr.end());
            if (matCat == engine::MaterialCat::UNKNOWN) {
                X_ERROR("Converter", "Unknown material cat: \"%*.s\"", typeStr.length(), typeStr.data());
                return false;
            }

            return true;
        }

        X_ERROR_IF(!slient, "Converter", "missing material cat");
        return false;
    }

    bool GetTechName(core::string& name, bool slient = false)
    {
        auto techNameStr = gEnv->pCore->GetCommandLineArg("name"_sv);
        if (techNameStr) {
            name.assign(techNameStr.begin(), techNameStr.end());
            return true;
        }

        X_ERROR_IF(!slient, "Converter", "missing tech name");
        return false;
    }

    bool ForceModeEnabled(void)
    {
        auto forceStr = gEnv->pCore->GetCommandLineArg("force"_sv);
        if (forceStr) {
            return core::strUtil::StringToBool(forceStr.begin(), forceStr.end());
        }
        return false;
    }

    bool DebugCompile(void)
    {
        auto debugStr = gEnv->pCore->GetCommandLineArg("debug"_sv);
        if (debugStr) {
            return core::strUtil::StringToBool(debugStr.begin(), debugStr.end());
        }
        return false;
    }

} // namespace

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nCmdShow)
{
    core::MallocFreeAllocator allocator;
    CompilerArena arena(&allocator, "ShaderCompilerArena");

    bool res = false;

    {
        EngineApp app; // needs to clear up before arena.

        if (app.Init(hInstance, &arena, lpCmdLine)) 
        {
            engine::compiler::TechDefCompiler con(&arena);
            engine::MaterialCat::Enum matCat;
            CompileMode::Enum mode;
            core::string techName;

            con.PrintBanner();

            if (con.Init()) {
                con.setForceCompile(ForceModeEnabled());
                
                auto flags = render::shader::COMPILE_BAKE_FLAGS;
                if (DebugCompile()) {
                    flags = render::shader::COMPILE_DEBUG_FLAGS;
                }
                
                con.setCompileFlags(flags);

                if (!GetMode(mode)) {
                    mode = CompileMode::SINGLE;
                }

                core::StopWatch timer;

                if (mode == CompileMode::CLEAN) {
                    if (!con.CleanAll()) {
                        X_ERROR("ShaderCompiler", "Failed to clean..");
                    }
                }
                else if (mode == CompileMode::ALL) {
                    if (GetMaterialCat(matCat, true)) {
                        if (!con.Compile(matCat)) {
                            X_ERROR("ShaderCompiler", "Compile failed..");
                        }
                        else {
                            res = true;
                        }
                    }
                    else {
                        if (!con.CompileAll()) {
                            X_ERROR("ShaderCompiler", "Compile failed..");
                        }
                        else {
                            res = true;
                        }
                    }
                }
                else if (GetMaterialCat(matCat) && GetTechName(techName)) {
                    if (!con.Compile(matCat, techName)) {
                        X_ERROR("ShaderCompiler", "Compile failed..");
                    }
                    else {
                        res = true;
                    }
                }

                X_LOG0("ShaderCompiler", "Elapsed time: ^6%gms", timer.GetMilliSeconds());
            }
            else {
                X_ERROR("ShaderCompiler", "Failed to init compiler");
            }

            gEnv->pConsoleWnd->pressToContinue();
        }
    }

    return res ? 0 : 1;
}
