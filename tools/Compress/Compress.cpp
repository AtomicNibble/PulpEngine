#include "stdafx.h"
#include "Compress.h"

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

namespace
{

	typedef core::MemoryArena<
		core::MallocFreeAllocator,
		core::SingleThreadPolicy,
		core::SimpleBoundsChecking,
		core::NoMemoryTracking,
		core::SimpleMemoryTagging
	> CompressorArena;


	struct FileInfo
	{
		FileInfo() {
			core::zero_this(this);
		}
		uint64_t deflatedSize;
		uint64_t inflatedSize;
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
			out.resize(info.inflatedSize);

			return T::inflate(arena, data, out);
		}

	};

	X_DECLARE_ENUM(Algo)(LZ4, LZMA, ZLIB);

	bool ReadFileToBuf(const std::string& filePath, core::Array<uint8_t>& bufOut, FileInfo* pFinfo = nullptr)
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

		bufOut.resize(size);

		if (file.read(reinterpret_cast<char*>(bufOut.ptr()), size))
		{
			return true;
		}
		return false;
	}

	bool WriteFileToBuf(const std::string& filePath, const core::Array<uint8_t>& buf, FileInfo* pInfo)
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

} // namespace 

core::MemoryArenaBase* g_arena = nullptr;


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	core::Console Console(L"Potato - Compressor");
	Console.RedirectSTD();
	Console.SetSize(60, 40, 2000);
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

	std::string inFile = R"(C:\Users\WinCat\Documents\code\WinCat\engine\potatoengine\game_folder\mod\models\mech_body.model_raw.lz4)";
	std::string outFile = R"(C:\Users\WinCat\Documents\code\WinCat\engine\potatoengine\game_folder\mod\models\mech_body.model_raw)";

	bool defalte = false;
	int32_t lvl = 5;

	Algo::Enum algo = Algo::LZ4;
	ICompressor* pCompressor = nullptr;

	if (algo == Algo::LZ4) {
		pCompressor = X_NEW(Compressor<core::Compression::LZ4>, g_arena, "LZ4");
		if (defalte) {
			outFile += ".lz4";
		}
	}
	else if (algo == Algo::LZMA) {
		pCompressor = X_NEW(Compressor<core::Compression::LZMA>, g_arena, "LZMA");
		if (defalte) {
			outFile += ".lzma";
		}
	}
	else if (algo == Algo::ZLIB) {
		pCompressor = X_NEW(Compressor<core::Compression::Zlib>, g_arena, "ZLIB");
		if (defalte) {
			outFile += ".zlib";
		}
	}
	else {
		return -1;
	}
 
	core::Array<uint8_t> inFileData(g_arena);
	core::Array<uint8_t> outfileData(g_arena);

	FileInfo info;
	if (!ReadFileToBuf(inFile, inFileData, defalte ? nullptr : &info)) {
		return -1;
	}

	bool res = false;

	core::StopWatch timer;

	if (defalte) {
		pCompressor->setLvl(lvl);
		res = pCompressor->deflate(g_arena, inFileData, info, outfileData);
	}
	else {
		res = pCompressor->inflate(g_arena, inFileData, info, outfileData);
	}

	float elapsedMS = timer.GetMilliSeconds();

	if (!res) {
		return -1;
	}

	if (!WriteFileToBuf(outFile, outfileData, defalte ? &info : nullptr)) {
		return -1;
	}


	X_DELETE(pCompressor, g_arena);
    return 0;
}
