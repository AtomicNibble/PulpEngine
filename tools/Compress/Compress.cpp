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

	X_DECLARE_ENUM(Algo)(LZ4, LZMA, ZLIB);

	struct FileInfo
	{
		FileInfo() {
			core::zero_this(this);
		}
		uint64_t deflatedSize;
		uint64_t inflatedSize;
		Algo::Enum algo;
	};


	struct ICompressor
	{
		 ICompressor() : lvl_(5) {}
		virtual ~ICompressor() {}

		virtual void setLvl(int32_t lvl) {
			lvl_ = lvl;
		}

		virtual bool deflate(core::MemoryArenaBase* arena, const core::Array<uint8_t>& data,
			FileInfo& info, core::Array<uint8_t>& out) X_ABSTRACT;
		virtual bool inflate(core::MemoryArenaBase* arena, const core::Array<uint8_t>& data,
			const FileInfo& info, core::Array<uint8_t>& out) X_ABSTRACT;

	protected:
		int32_t lvl_;
	};

	template<typename T>
	struct Compressor : public ICompressor
	{
		virtual bool deflate(core::MemoryArenaBase* arena, const core::Array<uint8_t>& data, 
			FileInfo& info, core::Array<uint8_t>& out) X_FINAL
		{
			T::CompressLevel::Enum lvl = T::CompressLevel::LOW;
			if (lvl_ > 3) {
				lvl = T::CompressLevel::NORMAL;
			}
			else if (lvl_ > 6) {
				lvl = T::CompressLevel::HIGH;
			}

			bool res = T::deflate(arena, data, out, lvl);
			if (res) {
				info.deflatedSize = out.size();
			}
			return res;
		}

		virtual bool inflate(core::MemoryArenaBase* arena, const core::Array<uint8_t>& data, 
			const FileInfo& info, core::Array<uint8_t>& out) X_FINAL
		{
			out.resize(safe_static_cast<size_t, uint64_t>(info.inflatedSize));

			return T::inflate(arena, data, out);
		}

	};


	bool ReadFileToBuf(const std::wstring& filePath, core::Array<uint8_t>& bufOut, FileInfo* pFinfo = nullptr)
	{
		std::ifstream file(filePath, std::ios::binary | std::ios::ate);
		std::streamsize size = file.tellg();
		file.seekg(0, std::ios::beg);

		if (pFinfo) {
			if (!file.read(reinterpret_cast<char*>(pFinfo), sizeof(*pFinfo))) {
				return false;
			}

			size -= sizeof(*pFinfo);
		}

		bufOut.resize(safe_static_cast<size_t,std::streamsize>(size));

		if (file.read(reinterpret_cast<char*>(bufOut.ptr()), size))
		{
			return true;
		}
		return false;
	}

	bool WriteFileToBuf(const std::wstring& filePath, const core::Array<uint8_t>& buf, FileInfo* pInfo)
	{
		std::ofstream file(filePath, std::ios::binary | std::ios::out);

		if (pInfo) {
			pInfo->deflatedSize = buf.size();

			file.write(reinterpret_cast<const char*>(pInfo), sizeof(*pInfo));
		}

		if (file.write(reinterpret_cast<const char*>(buf.ptr()), buf.size()))
		{
			return true;
		}
		return false;
	}


	void PrintArgs(void)
	{
		X_LOG0("Compressor", "Args:");
		X_LOG0("Compressor", "^6-if^7		(input file) ^1required");
		X_LOG0("Compressor", "^6-of^7		(output file, default: file + algo) ^9not-required");
		X_LOG0("Compressor", "^6-a^7		(algo 1:lz4 2:lzma 3:zlib, default: lza) ^1not-required");
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
	// lvl: 1-9

	EngineApp app;

	if (!app.Init(lpCmdLine, Console)) {
		return -1;
	}

	PrintArgs();

	std::wstring inFile, outFile;

	Algo::Enum algo = Algo::LZ4;
	bool defalte = true;
	int32_t lvl = 5;

	// args
	{
		const wchar_t* pInFile = gEnv->pCore->GetCommandLineArgForVarW(L"if");
		if (!pInFile) {
			X_ERROR("Compress", "Missing required arg -if");
			return -1;
		}

		inFile = pInFile;

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
				algo = Algo::LZMA;
			}
			else if (iAlgo == 3) {
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

			lvl = constrain<int32_t>(lvlArg, 1, 9);		
		}
	}

	core::Array<uint8_t> inFileData(g_arena);
	core::Array<uint8_t> outfileData(g_arena);

	core::StopWatch timer;
	FileInfo info;

	if (!ReadFileToBuf(inFile, inFileData, defalte ? nullptr : &info)) {
		X_ERROR("Compress", "Failed to read input file");
		return -1;
	}
	
	const float loadTime = timer.GetMilliSeconds();
	X_LOG0("Compress", "loadTime: ^2%fms", loadTime);

	// if infalting get algo from file.
	if (!defalte) {
		algo = info.algo;
	}

	ICompressor* pCompressor = nullptr;

	if (algo == Algo::LZ4) {
		pCompressor = X_NEW(Compressor<core::Compression::LZ4>, g_arena, "LZ4");
		if (defalte && outFile.empty()) {
			outFile = inFile + L".lz4";
			info.algo = Algo::LZ4;
		}
	}
	else if (algo == Algo::LZMA) {
		pCompressor = X_NEW(Compressor<core::Compression::LZMA>, g_arena, "LZMA");
		if (defalte && outFile.empty()) {
			outFile = inFile + L".lzma";
			info.algo = Algo::LZ4;
		}
	}
	else if (algo == Algo::ZLIB) {
		pCompressor = X_NEW(Compressor<core::Compression::Zlib>, g_arena, "ZLIB");
		if (defalte && outFile.empty()) {
			outFile = inFile + L".zlib";
			info.algo = Algo::LZ4;
		}
	}
	else {
		X_ERROR("Compress","unknown compression algo: %i", algo);
		return -1;
	}
 

	bool res = false;

	timer.Start();

	if (defalte) {
		pCompressor->setLvl(lvl);
		res = pCompressor->deflate(g_arena, inFileData, info, outfileData);
	}
	else {
		res = pCompressor->inflate(g_arena, inFileData, info, outfileData);
	}

	const float compressTime = timer.GetMilliSeconds();
	X_LOG0("Compress", "compressTime: ^2%fms", compressTime);

	if (!res) {
		X_ERROR("Compress", "%s failed.", defalte ? "deflation" : "inflation");
		return -1;
	}

	timer.Start();

	if (!WriteFileToBuf(outFile, outfileData, defalte ? &info : nullptr)) {
		X_ERROR("Compress", "Failed to write output file");
		return -1;
	}

	const float writeTime = timer.GetMilliSeconds();
	X_LOG0("Compress", "writeTime: ^2%fms", writeTime);


	X_DELETE(pCompressor, g_arena);
    return 0;
}
