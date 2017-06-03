#include "stdafx.h"
#include "Compress.h"
#include "EngineApp.h"

#define _LAUNCHER
#include <ModuleExports.h>

#include <Platform\Console.h>

#include <Compression\LZ4.h>
#include <Compression\Lzma2.h>
#include <Compression\Zlib.h>
#include <Compression\Store.h>

#include <Time\StopWatch.h>

#include <istream>
#include <iostream>
#include <fstream>

#include <ICompression.h>

#ifdef X_LIB

struct XRegFactoryNode* g_pHeadToRegFactories = nullptr;

X_LINK_LIB("engine_Core")

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
	> CompressorArena;

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

		bufOut.resize(safe_static_cast<size_t,std::streamsize>(size));

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
				iAlgo = constrain<int32_t>(iAlgo, 1, 3);

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
			algo = ICompressor::getAlgo(inFileData);
		}

		ICompressor* pCompressor = nullptr;

		static_assert(core::Compression::Algo::ENUM_COUNT == 7, "Added additional compression algos? this code needs updating.");

		switch (algo)
		{
			case Algo::LZ4:
				pCompressor = X_NEW(Compressor<core::Compression::LZ4>, &arena, "LZ4");
				if (defalte && outFile.empty()) {
					outFile = inFile + L".lz4";
				}
				break;
			case Algo::LZ4HC:
				pCompressor = X_NEW(Compressor<core::Compression::LZ4HC>, &arena, "LZ4HC");
				if (defalte && outFile.empty()) {
					outFile = inFile + L".lz4";
				}
				break;
			case Algo::LZ5:
				pCompressor = X_NEW(Compressor<core::Compression::LZ4>, &arena, "LZ4");
				if (defalte && outFile.empty()) {
					outFile = inFile + L".lz4";
				}
				break;
			case Algo::LZ5HC:
				pCompressor = X_NEW(Compressor<core::Compression::LZ4HC>, &arena, "LZ4HC");
				if (defalte && outFile.empty()) {
					outFile = inFile + L".lz4";
				}
				break;
			case Algo::LZMA:
				pCompressor = X_NEW(Compressor<core::Compression::LZMA>, &arena, "LZMA");
				if (defalte && outFile.empty()) {
					outFile = inFile + L".lzma";
				}
				break;
			case Algo::ZLIB:
				pCompressor = X_NEW(Compressor<core::Compression::Zlib>, &arena, "ZLIB");
				if (defalte && outFile.empty()) {
					outFile = inFile + L".zlib";
				}
				break;
			case Algo::STORE:
				pCompressor = X_NEW(Compressor<core::Compression::Store>, &arena, "Store");
				if (defalte && outFile.empty()) {
					outFile = inFile + L".store";
				}
				break;
			default:
				X_ERROR("Compress", "unknown compression algo: %i", algo);
				return -1;
		}

		bool res = false;

		timer.Start();

		if (defalte) {
			res = pCompressor->deflate(&arena, inFileData, outfileData, lvl);
		}
		else {
			res = pCompressor->inflate(&arena, inFileData, outfileData);
		}

		const float compressTime = timer.GetMilliSeconds();
		X_LOG0("Compress", "%s: ^2%fms", defalte ? "deflateTime" : "inflateTime", compressTime);

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


		X_DELETE(pCompressor, &arena);
		return 0;
	}

	int DoTrain(CompressorArena& arena)
	{
		X_UNUSED(arena);


		return 0;
	}

} // namespace 

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
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

	if (1)
	{
		res = DoCompression(arena);
	}
	else
	{
		res = DoTrain(arena);
	}


    return res;
}
