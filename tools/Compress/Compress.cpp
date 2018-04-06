#include "stdafx.h"
#include "Compress.h"
#include "EngineApp.h"

#define _LAUNCHER
#include <ModuleExports.h>

#include <Platform\Console.h>

#include <Compression\CompressorAlloc.h>
#include <Compression\DictBuilder.h>

#include <String\HumanSize.h>
#include <String\HumanDuration.h>

#include <Time\StopWatch.h>

#include <Util\UniquePointer.h>

#include <istream>
#include <iostream>
#include <fstream>

#include <ICompression.h>
#include <IFileSys.h>

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
        CompressorArena;

    using core::Compression::CompressLevel;
    using core::Compression::Algo;
    using core::Compression::BufferHdr;
    using core::Compression::ICompressor;
    using core::Compression::Compressor;

    bool ReadFileToBuf(const std::wstring& filePath, core::Array<uint8_t>& bufOut)
    {
        std::ifstream file(filePath, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            X_ERROR("Compress", "Failed to open input file: \"%ls\"", filePath.c_str());
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
            X_ERROR("Compress", "Failed to open output file: \"%ls\"", filePath.c_str());
            return false;
        }

        if (file.write(reinterpret_cast<const char*>(buf.ptr()), buf.size())) {
            return true;
        }
        return false;
    }

    void PrintArgs(void)
    {
        static_assert(core::Compression::Algo::ENUM_COUNT == 7, "Added additional compression algos? this code needs updating.");

        X_LOG0("Compressor", "Args:");
        X_LOG0("Compressor", "^6-if^7		(input file) ^1required");
        X_LOG0("Compressor", "^6-of^7		(output file, default: file + algo) ^9not-required");
        X_LOG0("Compressor", "^6-a^7		(algo 1:lz4 2:lz4hc 3:lz5 4:lz5hc 5:lzma 6:zlib 7:store, default: lz4) ^9not-required");
        X_LOG0("Compressor", "^6-d^7		(deflate 1/0, default: 1) ^9not-required");
        X_LOG0("Compressor", "^6-lvl^7		(lvl 1-9, default: 5) ^9not-required");
        X_LOG0("Compressor", "TrainArgs:");
        X_LOG0("Compressor", "^6-train^7		enable training mode ^9not-required");
        X_LOG0("Compressor", "^6-train-src^7	source dir for samples ^1required");
        X_LOG0("Compressor", "^6-train-of^7		output file for dict ^1required");
        X_LOG0("Compressor", "^6-train-md^7		max size of output dict(default: 64k) ^9not-required");
    }

    int DoCompression(CompressorArena& arena)
    {
        std::wstring inFile, outFile;

        Algo::Enum algo = Algo::LZ4;
        bool defalte = true;
        CompressLevel::Enum lvl = CompressLevel::NORMAL;

        // args
        {
            const wchar_t* pInFile = gEnv->pCore->GetCommandLineArgForVarW(L"if");
            if (!pInFile) {
                int32_t numArgs = __argc;
                if (numArgs == 2) {
                    pInFile = __wargv[1];
                    outFile = pInFile;
                    outFile += L".out";
                    defalte = false;
                }
                else {
                    X_ERROR("Compress", "Missing required arg -if");
                    return -1;
                }
            }

            inFile = pInFile;

            const wchar_t* pMode = gEnv->pCore->GetCommandLineArgForVarW(L"mode");
            if (pMode) {
                if (core::strUtil::IsEqualCaseInsen(pMode, L"inflate")) {
                    defalte = false;
                }
            }

            const wchar_t* pOutFile = gEnv->pCore->GetCommandLineArgForVarW(L"of");
            if (pOutFile) {
                outFile = pOutFile;
            }

            const wchar_t* pAlgo = gEnv->pCore->GetCommandLineArgForVarW(L"a");
            if (pAlgo) {
                int32_t iAlgo = core::strUtil::StringToInt<int32_t>(pAlgo);
                iAlgo = constrain<int32_t>(iAlgo, 1, Algo::ENUM_COUNT);

                static_assert(core::Compression::Algo::ENUM_COUNT == 7, "Added additional compression algos? this code needs updating.");

                if (iAlgo == 1) {
                    algo = Algo::LZ4;
                }
                else if (iAlgo == 2) {
                    algo = Algo::LZ4HC;
                }
                else if (iAlgo == 3) {
                    algo = Algo::LZ5;
                }
                else if (iAlgo == 4) {
                    algo = Algo::LZ5HC;
                }
                else if (iAlgo == 5) {
                    algo = Algo::LZMA;
                }
                else if (iAlgo == 6) {
                    algo = Algo::ZLIB;
                }
                else if (iAlgo == 7) {
                    algo = Algo::STORE;
                }
                else {
                    X_ASSERT_UNREACHABLE();
                    return -1;
                }
            }

            const wchar_t* pLvl = gEnv->pCore->GetCommandLineArgForVarW(L"lvl");
            if (pLvl) {
                int32_t lvlArg = core::strUtil::StringToInt<int32_t>(pLvl);
                lvlArg = constrain<int32_t>(lvlArg, 1, CompressLevel::ENUM_COUNT) - 1;

                lvl = static_cast<CompressLevel::Enum>(lvlArg);
            }
        }

        if (!defalte && outFile.empty()) {
            X_ERROR("Compress", "Output file name missing.");
            return 1;
        }

        core::Array<uint8_t> inFileData(&arena);
        core::Array<uint8_t> outfileData(&arena);

        core::StopWatch timer;

        if (!ReadFileToBuf(inFile, inFileData)) {
            X_ERROR("Compress", "Failed to read input file");
            return -1;
        }

        const float loadTime = timer.GetMilliSeconds();
        X_LOG0("Compress", "loadTime: ^2%fms", loadTime);

        // if infalting get algo from buf.
        if (!defalte) {
            if (!ICompressor::validBuffer(inFileData)) {
                X_ERROR("Compress", "Input file is not a compressed buffer");
                return -1;
            }

            algo = ICompressor::getAlgo(inFileData);
        }
        else {
            X_LOG1("Compress", "Deflating Algo: %s lvl: %s", Algo::ToString(algo), CompressLevel::ToString(lvl));
        }

        core::Compression::CompressorAlloc compressor(algo);

        // auto out file name.
        if (defalte && outFile.empty()) {
            switch (algo) {
                case Algo::LZ4:
                    outFile = inFile + L".lz4";
                    break;
                case Algo::LZ4HC:
                    outFile = inFile + L".lz4";
                    break;
                case Algo::LZ5:
                    outFile = inFile + L".lz5";
                    break;
                case Algo::LZ5HC:
                    outFile = inFile + L".lz5";
                    break;
                case Algo::LZMA:
                    outFile = inFile + L".lzma";
                    break;
                case Algo::ZLIB:
                    outFile = inFile + L".zlib";
                    break;
                case Algo::STORE:
                    outFile = inFile + L".store";
                    break;
                default:
                    X_ERROR("Compress", "unknown compression algo: %i", algo);
                    return -1;
            }
        }

        bool res = false;

        timer.Start();

        if (defalte) {
            res = compressor->deflate(&arena, inFileData, outfileData, lvl);
        }
        else {
            res = compressor->inflate(&arena, inFileData, outfileData);
        }

        const float compressTime = timer.GetMilliSeconds();

        core::HumanDuration::Str durStr;
        X_LOG0("Compress", "%s: ^2%s", defalte ? "deflateTime" : "inflateTime", core::HumanDuration::toString(durStr, compressTime));

        if (!res) {
            X_ERROR("Compress", "%s failed.", defalte ? "deflation" : "inflation");
            return -1;
        }

        timer.Start();

        if (!WriteFileFromBuf(outFile, outfileData)) {
            X_ERROR("Compress", "Failed to write output file");
            return -1;
        }

        const float writeTime = timer.GetMilliSeconds();
        X_LOG0("Compress", "writeTime: ^2%fms", writeTime);

        return 0;
    }

    int DoTrain(CompressorArena& arena)
    {
        core::Path<wchar_t> srcDir, outFile;
        size_t maxDictSize = std::numeric_limits<uint16_t>::max();

        const wchar_t* pSrcDir = gEnv->pCore->GetCommandLineArgForVarW(L"train-src");
        if (pSrcDir) {
            srcDir = pSrcDir;
            srcDir.ensureSlash();
        }
        const wchar_t* pOutFile = gEnv->pCore->GetCommandLineArgForVarW(L"train-of");
        if (pOutFile) {
            outFile = pOutFile;
        }
        const wchar_t* pMaxDictSize = gEnv->pCore->GetCommandLineArgForVarW(L"train-md");
        if (pMaxDictSize) {
            maxDictSize = core::strUtil::StringToInt<size_t>(pMaxDictSize);
        }

        if (srcDir.isEmpty()) {
            X_ERROR("Train", "Source dir is empty");
            return -1;
        }
        if (outFile.isEmpty()) {
            X_ERROR("Train", "Output file name missing.");
            return -1;
        }
        if (maxDictSize < 256) {
            X_ERROR("Train", "Invalid maxDictSize must be atleast 256");
            return -1;
        }

        // we need to load all the files in the src directory and merge them into a single buffer.
        // so first we should just get all the sizes.
        core::Array<size_t> sampleSizes(&arena);
        sampleSizes.setGranularity(256);

        core::Array<uint8_t> sampleData(&arena);
        core::Array<uint8_t> dictData(&arena);

        // process the files.
        {
            core::Array<core::Path<wchar_t>> fileNames(&arena);
            fileNames.setGranularity(256);

            core::Path<wchar_t> pathW(pSrcDir);
            core::Path<char> path(pathW);
            path.ensureSlash();
            path.append("*");

            X_LOG0("Train", "Gathering files for training");

            core::FindFirstScoped find;
            if (find.findfirst(path.c_str())) {
                do {
                    const auto& fd = find.fileData();
                    if (fd.attrib & FILE_ATTRIBUTE_DIRECTORY) {
                        continue;
                    }

                    const size_t sampleSize = core::Min<size_t>(safe_static_cast<size_t>(fd.size), core::Compression::DICT_SAMPLER_SIZE_MAX);
                    if (sampleSize == 0) {
                        continue;
                    }

                    sampleSizes.push_back(sampleSize);
                    fileNames.emplace_back(fd.name);

                } while (find.findNext());
            }

            X_ASSERT(sampleSizes.size() == fileNames.size(), "Array sizes should match")
            ();

            if (fileNames.size() < core::Compression::DICT_SAMPLER_MIN_SAMPLES) {
                X_ERROR("Train", "Only %" PRIuS " samples provided, atleast %" PRIuS " required",
                    fileNames.size(), core::Compression::DICT_SAMPLER_MIN_SAMPLES);
                return -1;
            }

            const size_t totalSampleSize = core::accumulate(sampleSizes.begin(), sampleSizes.end(), 0_sz);
            sampleData.resize(totalSampleSize);

            size_t currentOffset = 0;

            X_LOG0("Train", "Loading ^6%" PRIuS "^7 file(s) data for training", fileNames.size());

            // load all the files.
            for (size_t i = 0; i < fileNames.size(); i++) {
                core::Path<wchar_t> filePath(srcDir);
                filePath.appendFmt(L"%s", fileNames[i].c_str());

                std::ifstream file(filePath.c_str(), std::ios::binary);
                if (!file.is_open()) {
                    X_ERROR("Train", "Failed to open input file: \"%ls\"", filePath.c_str());
                    continue;
                }

                file.read(reinterpret_cast<char*>(&sampleData[currentOffset]), sampleSizes[i]);
                currentOffset += sampleSizes[i];
            }

            X_ASSERT(currentOffset == sampleData.size(), "Error reading sample data")
            (currentOffset, sampleData.size());
        }

        // train.
        core::HumanSize::Str sizeStr, sizeStr1;
        X_LOG0("Train", "Training with ^6%s^7 sample data, ^6%" PRIuS "^7 files, avg size: ^6%s",
            core::HumanSize::toString(sizeStr, sampleData.size()),
            sampleSizes.size(),
            core::HumanSize::toString(sizeStr1, sampleData.size() / sampleSizes.size()));

        core::StopWatch timer;

        if (!core::Compression::trainDictionary(sampleData, sampleSizes, dictData, maxDictSize)) {
            X_ERROR("Train", "Fail to train dictionary");
            return -1;
        }

        const float trainTime = timer.GetMilliSeconds();
        core::HumanDuration::Str timeStr;
        X_LOG0("Train", "Train took: ^6%s", core::HumanDuration::toString(timeStr, trainTime));

        std::ofstream file(outFile.c_str(), std::ios::binary | std::ios::out);
        if (!file.is_open()) {
            X_ERROR("Train", "Failed to open output file: \"%ls\"", outFile.c_str());
            return -1;
        }

        // make the file be the size of the requested dict.
        core::Compression::SharedDictHdr hdr;

        const auto pStart = &dictData[sizeof(hdr)];
        const auto size = dictData.size() - sizeof(hdr);

        hdr.magic = core::Compression::SharedDictHdr::MAGIC;
        hdr.sharedDictId = gEnv->xorShift.rand() & 0xFFFF;
        hdr.size = safe_static_cast<uint32_t>(size);

        file.write(reinterpret_cast<const char*>(&hdr), sizeof(hdr));
        file.write(reinterpret_cast<const char*>(pStart), size);

        return 0;
    }

} // namespace

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nCmdShow)
{
    core::Console Console(X_WIDEN(X_ENGINE_NAME) L" - Compressor");
    Console.RedirectSTD();
    Console.SetSize(100, 40, 2000);
    Console.MoveTo(10, 10);

    core::MallocFreeAllocator allocator;
    CompressorArena arena(&allocator, "CompressorArena");

    // this tool is just going to wrap engine compression functionality.
    // basically just glue code, can be useful.

    // args:
    // in: file in
    // out: file out
    // mode: defalte infalte
    // algo: lz4, lzma, zlib
    // lvl: 1-3

    EngineApp app;

    if (!app.Init(hInstance, &arena, lpCmdLine, Console)) {
        return -1;
    }

    PrintArgs();

    int res = 0;

    const wchar_t* pTrain = gEnv->pCore->GetCommandLineArgForVarW(L"train");
    if (pTrain && core::strUtil::StringToBool(pTrain)) {
        res = DoTrain(arena);
    }
    else {
        res = DoCompression(arena);
    }

    return res;
}
