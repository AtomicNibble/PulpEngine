#include "stdafx.h"
#include "ImgTool.h"
#include "EngineApp.h"

#define _LAUNCHER
#include <ModuleExports.h>

#include <Platform\Console.h>

#include <String\HumanSize.h>
#include <String\HumanDuration.h>

#include <Time\StopWatch.h>
#include <Util\UniquePointer.h>

#include <../ImgLib/ImgLib.h>

#include <istream>
#include <iostream>
#include <fstream>

#include <IFileSys.h>

X_LINK_ENGINE_LIB("ImgLib")


#ifdef X_LIB

struct XRegFactoryNode* g_pHeadToRegFactories = nullptr;

X_LINK_ENGINE_LIB("Core")

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
        >
        ImgToolArena;


    bool ReadFileToBuf(const std::wstring& filePath, core::Array<uint8_t>& bufOut)
    {
        std::ifstream file(filePath, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            X_ERROR("ImgTool", "Failed to open input file: \"%ls\"", filePath.c_str());
            return false;
        }

        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        bufOut.resize(safe_static_cast<size_t, std::streamsize>(size));

        if (file.read(reinterpret_cast<char*>(bufOut.ptr()), size)) {
            return true;
        }
        return false;
    }

    bool WriteFileFromBuf(const std::wstring& filePath, const core::Array<uint8_t>& buf)
    {
        std::ofstream file(filePath, std::ios::binary | std::ios::out);
        if (!file.is_open()) {
            X_ERROR("ImgTool", "Failed to open output file: \"%ls\"", filePath.c_str());
            return false;
        }

        if (file.write(reinterpret_cast<const char*>(buf.ptr()), buf.size())) {
            return true;
        }
        return false;
    }

    void PrintArgs(void)
    {
        X_LOG0("ImgTool", "Args:");
      
        // TODO
    }

    bool Process()
    {


        return true;
    }

} // namespace

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nCmdShow)
{
    core::Console Console(X_WIDEN(X_ENGINE_NAME) L" - ImgTool");
    Console.RedirectSTD();
    Console.SetSize(100, 40, 2000);
    Console.MoveTo(10, 10);

    core::MallocFreeAllocator allocator;
    ImgToolArena arena(&allocator, "ImgToolArena");


    EngineApp app;

    if (!app.Init(hInstance, &arena, lpCmdLine, Console)) {
        return -1;
    }

    PrintArgs();

    if (!Process()) {
        return -1;
    }
   
    return 0;
}
