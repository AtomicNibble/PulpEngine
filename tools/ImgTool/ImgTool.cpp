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


    bool ReadFileToBuf(const core::Path<wchar_t>& filePath, core::Array<uint8_t>& bufOut)
    {
        core::XFileScoped file;
        if (!file.openFileOS(filePath, core::FileFlag::READ | core::FileFlag::SHARE)) {
            X_ERROR("ImgTool", "Failed to open input file: \"%ls\"", filePath.c_str());
            return false;
        }

        auto size = safe_static_cast<size_t>(file.remainingBytes());
        bufOut.resize(size);

        if (file.read(bufOut.ptr(), size) != size) {
            return false;
        }

        return true;
    }

    bool WriteFileFromBuf(const core::Path<wchar_t>& filePath, const core::Array<uint8_t>& buf)
    {
        core::XFileScoped file;
        if (!file.openFileOS(filePath, core::FileFlag::WRITE | core::FileFlag::RECREATE)) {
            X_ERROR("ImgTool", "Failed to open input file: \"%ls\"", filePath.c_str());
            return false;
        }

        if (file.write(buf.ptr(), buf.size()) != buf.size()) {
            return false;
        }

        return true;
    }

    void PrintArgs(void)
    {
        X_LOG0("ImgTool", "Args:");
        X_LOG0("ImgTool", "^6-if^7		(input file) ^1required");
        X_LOG0("ImgTool", "^6-of^7		(output file, default: file + fmt ext) ^9not-required");
        X_LOG0("ImgTool", "^6-dim^7		(dimensions) ^9not-required");
        X_LOG0("ImgTool", "^6-fmt^7		(output file format) ^9not-required");
    }

} // namespace

X_NAMESPACE_BEGIN(texture)

namespace
{
   

    bool Process(ImgToolArena& arena)
    {
        // currently i want a tool that will take N input images and resize and save as tga.
        // so need input file, output shit, desired size and format.

        core::Path<wchar_t> inFile, outFile;
        Vec2<uint16_t> dim;

        bool resize = false;

        ImgFileFormat::Enum outputFileFmt = ImgFileFormat::TGA;

        // args
        {
            const wchar_t* pInFile = gEnv->pCore->GetCommandLineArgForVarW(L"if");
            if (!pInFile) {
                X_ERROR("ImgTool", "Missing required arg -if");
                return false;
            }

            inFile.set(pInFile);

            const wchar_t* pOutFile = gEnv->pCore->GetCommandLineArgForVarW(L"of");
            if (pOutFile) {
                outFile.set(pOutFile);
            }

            const wchar_t* pDim = gEnv->pCore->GetCommandLineArgForVarW(L"dim");
            if (pDim) {
         
                // wXh
                if (::swscanf_s(pDim, L"%" PRIu16 L"x%" PRIu16, &dim.x, &dim.y) != 2) {
                    X_ERROR("ImgTool", "Failed to parse dim: %ls", pDim);
                    return false;
                }

                resize = true;
            }

            const wchar_t* pOutFmt = gEnv->pCore->GetCommandLineArgForVarW(L"fmt");
            if (pOutFmt) {

                char buf[64];
                outputFileFmt = texture::Util::ImgFileFmtFromStr(core::strUtil::Convert(pOutFmt, buf));
                if (outputFileFmt == ImgFileFormat::UNKNOWN) {
                    X_LOG0("ImgTool", "Unknown extension: \"%ls\"", pOutFmt);
                    return false;
                }
            }
        }

        if (outFile.isEmpty())
        {
            const char* pExt = texture::Util::getExtension(outputFileFmt);
            wchar_t buf[0x100] = {};

            outFile = inFile;
            outFile.setExtension(core::strUtil::Convert(pExt, buf));
        }

        // output exists?
        if (gEnv->pFileSys->fileExistsOS(outFile)) {
            X_WARNING("ImgTool", "Target already exsists skipping");
            return true;
        }

        X_LOG0("ImgTool", "Loading: \"%ls\"", inFile.c_str());

        core::Array<uint8_t> srcImgData(&arena);
        if (!ReadFileToBuf(inFile, srcImgData)) {
            return false; 
        }

        ImgFileFormat::Enum inputFileFmt = Util::resolveSrcfmt(srcImgData);
        if (inputFileFmt == ImgFileFormat::UNKNOWN) {
            X_ERROR("ImgTool", "Unknown img src format");
            return false;
        }

        Converter::ImgConveter con(&arena, &arena);

        if (!con.loadImg(srcImgData, inputFileFmt)) {
            return false;
        }

        {
            const auto& src = con.getTextFile();
            auto srcDim = src.getSize();

            X_LOG0("ImgTool", "Fmt: %s", Texturefmt::ToString(src.getFormat()));
            X_LOG0("ImgTool", "Dim: (^6%" PRIu16 "^7,^6 %" PRIu16 "^7)", srcDim.x, srcDim.y);

        }

        // scale me down baby!
        if(resize)
        {
            const auto& src = con.getTextFile();
            auto srcDim = src.getSize();

            if (srcDim.x > dim.x || srcDim.y > dim.y)
            {
                // keep ratio
                auto targetDim = srcDim;
                while (targetDim.x > dim.x || targetDim.y > dim.y)
                {
                    targetDim.x >>= 1;
                    targetDim.y >>= 1;

                    targetDim.x = core::Max(targetDim.x, 1_ui16);
                    targetDim.y = core::Max(targetDim.y, 1_ui16);
                }

                Converter::MipFilter::Enum mipFilter = Converter::MipFilter::Kaiser;
                Converter::WrapMode::Enum wrapMode = Converter::WrapMode::Clamp;

                if (!con.resize(targetDim, mipFilter, wrapMode)) {
                    X_ERROR("ImgTool", "Failed to create mips for image");
                    return false;
                }
            }
        }

        Converter::CompileFlags flags;

        X_LOG0("ImgTool", "Saving: \"%ls\"", outFile.c_str());

        core::FileFlags mode;
        mode.Set(core::FileFlag::WRITE);
        mode.Set(core::FileFlag::RECREATE);

        core::XFileScoped file;
        if (!file.openFileOS(outFile, mode)) {
            return false;
        }

        if (!con.saveImg(file.GetFile(), flags, outputFileFmt)) {
            return false;
        }

        return true;
    }

} // namespace

X_NAMESPACE_END

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nCmdShow)
{
    core::MallocFreeAllocator allocator;
    ImgToolArena arena(&allocator, "ImgToolArena");

    EngineApp app;

    if (!app.Init(hInstance, &arena, lpCmdLine)) {
        return 1;
    }

    PrintArgs();

    core::StopWatch timer;

    if (!texture::Process(arena)) {
        gEnv->pConsoleWnd->pressToContinue();
        return 1;
    }
   
    const float trainTime = timer.GetMilliSeconds();
    core::HumanDuration::Str timeStr;
    X_LOG0("Imgtool", "Processing took: ^6%s", core::HumanDuration::toString(timeStr, trainTime));

    return 0;
}
