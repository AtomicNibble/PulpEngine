#include "stdafx.h"
#include "ScriptCmd.h"
#include "EngineApp.h"

#define _LAUNCHER
#include <ModuleExports.h>

#include <Platform\Console.h>

#include <IFileSys.h>
#include <IScriptSys.h>

#ifdef X_LIB

struct XRegFactoryNode* g_pHeadToRegFactories = nullptr;

X_LINK_ENGINE_LIB("Core")

#endif // !X_LIB

using namespace core::string_view_literals;

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
        >
        ScriptCmdArena;

    X_DECLARE_ENUM(ScriptMode)
    (
        CHECK,
        RUN,
        BAKE);

    bool GetMode(ScriptMode::Enum& mode)
    {
        auto modeStr = gEnv->pCore->GetCommandLineArg("mode"_sv);
        if (modeStr) {
            if (core::strUtil::IsEqualCaseInsen(modeStr, "check"_sv)) {
                mode = ScriptMode::CHECK;
            }
            else if (core::strUtil::IsEqualCaseInsen(modeStr, "run"_sv)) {
                mode = ScriptMode::RUN;
            }
            else if (core::strUtil::IsEqualCaseInsen(modeStr, "bake"_sv)) {
                mode = ScriptMode::BAKE;
            }
            else {
                X_ERROR("Converter", "Unknown mode: \"%*.s\"", modeStr.length(), modeStr.data());
                return false;
            }

            return true;
        }

        return false;
    }

    bool GetInputFile(core::Path<char>& inputFile)
    {
        auto inputFileStr = gEnv->pCore->GetCommandLineArg("if"_sv);
        if (inputFileStr) {
            inputFile.set(inputFileStr.begin(), inputFileStr.end());
            return true;
        }

        X_ERROR("Converter", "missing input file");
        return false;
    }

} // namespace

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nCmdShow)
{
    core::MallocFreeAllocator allocator;
    ScriptCmdArena arena(&allocator, "ScriptCmdArena");

    int res = -1;

    {
        EngineApp app;

        if (app.Init(hInstance, &arena, lpCmdLine)) 
        {
            script::IScriptSys* pScriptSys = gEnv->pScriptSys;

            core::Path<char> inputFile;
            ScriptMode::Enum mode;

            if (!GetMode(mode)) {
                mode = ScriptMode::RUN;
            }

            if (!GetInputFile(inputFile)) {
                return 1;
            }

            core::Path<char> inputPath;

            inputPath = "scripts";
            inputPath /= inputFile;

            if (mode == ScriptMode::RUN) {
                core::XFileMemScoped file;

                if (!file.openFile(inputPath, core::FileFlag::READ | core::FileFlag::SHARE)) {
                    X_ERROR("Script", "Failed to open file: \"%s\"", inputFile.c_str());
                    return 1;
                }

                if (!pScriptSys->runScriptInSandbox(file->getBufferStart(), file->getBufferEnd())) {
                    X_ERROR("Script", "Error running script: \"%s\"", inputFile.c_str());
                    return 1;
                }

                return 0;
            }
            else if (mode == ScriptMode::CHECK) {
                X_ASSERT_NOT_IMPLEMENTED();
            }
            else if (mode == ScriptMode::BAKE) {
                X_ASSERT_NOT_IMPLEMENTED();
            }
        }
    }

    return res;
}
