#include "stdafx.h"
#include "Compress.h"
#include "EngineApp.h"

#define _LAUNCHER
#include <ModuleExports.h>

#include <Platform\Console.h>

#include <Compression\LZ4.h>
#include <Compression\Lzma2.h>
#include <Compression\Zlib.h>

#include <Time\StopWatch.h>

#include <istream>
#include <iostream>
#include <fstream>

#include <ICompression.h>

#ifdef X_LIB

struct XRegFactoryNode* g_pHeadToRegFactories = 0;

X_LINK_LIB("engine_Core")

#endif // !X_LIB

namespace
{

	typedef core::MemoryArena<
		core::MallocFreeAllocator,
		core::SingleThreadPolicy,
		core::SimpleBoundsChecking,
		core::NoMemoryTracking,
		core::SimpleMemoryTagging
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

	bool WriteFileToBuf(const std::wstring& filePath, const core::Array<uint8_t>& buf)
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
		X_LOG0("Compressor", "Args:");
		X_LOG0("Compressor", "^6-if^7		(input file) ^1required");
		X_LOG0("Compressor", "^6-of^7		(output file, default: file + algo) ^9not-required");
		X_LOG0("Compressor", "^6-a^7		(algo 1:lz4 2:lz4hc 3:lzma 4:zlib, default: lza) ^9not-required");
		X_LOG0("Compressor", "^6-d^7		(deflate 1/0, default: 1) ^9not-required");
		X_LOG0("Compressor", "^6-lvl^7		(lvl 1-9, default: 5) ^9not-required");
	}

} // namespace 

core::MemoryArenaBase* g_arena = nullptr;
HINSTANCE g_hInstance = 0;


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	g_hInstance = hInstance;

	core::Console Console(L"Potato - Compressor");
	Console.RedirectSTD();
	Console.SetSize(100, 40, 2000);
	Console.MoveTo(10, 10);


	core::MallocFreeAllocator allocator;
	CompressorArena arena(&allocator, "CompressorArena");
	g_arena = &arena;

	// this tool is just going to wrap engine compression functionality.
	// basically just glue code, can be useful.

	// args:
	// in: file in
	// out: file out
	// mode: defalte infalte
	// algo: lz4, lzma, zlib
	// lvl: 1-3

	EngineApp app;

	if (!app.Init(lpCmdLine, Console)) {
		return -1;
	}

	PrintArgs();

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

			if (iAlgo == 1) {
				algo = Algo::LZ4;
			}
			else if (iAlgo == 2) {
				algo = Algo::LZ4HC;
			}
			else if (iAlgo == 3) {
				algo = Algo::LZMA;
			}
			else if (iAlgo == 4) {
				algo = Algo::ZLIB;
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

	core::Array<uint8_t> inFileData(g_arena);
	core::Array<uint8_t> outfileData(g_arena);

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

	if (algo == Algo::LZ4) {
		pCompressor = X_NEW(Compressor<core::Compression::LZ4>, g_arena, "LZ4");
		if (defalte && outFile.empty()) {
			outFile = inFile + L".lz4";
		}
	}
	else if (algo == Algo::LZ4HC) {
		pCompressor = X_NEW(Compressor<core::Compression::LZ4HC>, g_arena, "LZ4HC");
		if (defalte && outFile.empty()) {
			outFile = inFile + L".lz4";
		}
	}
	else if (algo == Algo::LZMA) {
		pCompressor = X_NEW(Compressor<core::Compression::LZMA>, g_arena, "LZMA");
		if (defalte && outFile.empty()) {
			outFile = inFile + L".lzma";
		}
	}
	else if (algo == Algo::ZLIB) {
		pCompressor = X_NEW(Compressor<core::Compression::Zlib>, g_arena, "ZLIB");
		if (defalte && outFile.empty()) {
			outFile = inFile + L".zlib";
		}
	}
	else {
		X_ERROR("Compress","unknown compression algo: %i", algo);
		return -1;
	}
 

	bool res = false;

	timer.Start();

	if (defalte) {
		res = pCompressor->deflate(g_arena, inFileData, outfileData, lvl);
	}
	else {
		res = pCompressor->inflate(g_arena, inFileData, outfileData);
	}

	const float compressTime = timer.GetMilliSeconds();
	X_LOG0("Compress", "%s: ^2%fms", defalte ? "deflateTime" : "inflateTime", compressTime);

	if (!res) {
		X_ERROR("Compress", "%s failed.", defalte ? "deflation" : "inflation");
		return -1;
	}

	timer.Start();

	if (!WriteFileToBuf(outFile, outfileData)) {
		X_ERROR("Compress", "Failed to write output file");
		return -1;
	}

	const float writeTime = timer.GetMilliSeconds();
	X_LOG0("Compress", "writeTime: ^2%fms", writeTime);


	X_DELETE(pCompressor, g_arena);
    return 0;
}
