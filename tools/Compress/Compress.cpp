#include "stdafx.h"
#include "Compress.h"

#define _LAUNCHER
#include <ModuleExports.h>

#include <Platform\Console.h>

#include <Compression\LZ4.h>
#include <Compression\Lzma2.h>
#include <Compression\Zlib.h>

#include <Time\StopWatch.h>

namespace
{

	typedef core::MemoryArena<
		core::MallocFreeAllocator,
		core::SingleThreadPolicy,
		core::SimpleBoundsChecking,
		core::NoMemoryTracking,
		core::SimpleMemoryTagging
	> CompressorArena;



	struct ICompressor
	{
		virtual ~ICompressor() {}

		virtual bool deflate(const core::Array<uint8_t>& data, core::Array<uint8_t>& out) X_ABSTRACT;
		virtual bool inflate(const core::Array<uint8_t>& data, core::Array<uint8_t>& out) X_ABSTRACT;
	};


	class LZ4 : public ICompressor
	{
		virtual bool deflate(const core::Array<uint8_t>& data, core::Array<uint8_t>& out) X_FINAL
		{
			return core::Compression::LZ4::deflate(data, out, core::Compression::LZ4::CompressLevel::LOW);
		}

		virtual bool inflate(const core::Array<uint8_t>& data, core::Array<uint8_t>& out) X_FINAL
		{
			return core::Compression::LZ4::inflate(data, out);
		}
	};

	class LZMA : public ICompressor
	{
		virtual bool deflate(const core::Array<uint8_t>& data, core::Array<uint8_t>& out) X_FINAL
		{
			return core::Compression::LZMA::deflate(data, out, core::Compression::LZMA::CompressLevel::LOW);
		}

		virtual bool inflate(const core::Array<uint8_t>& data, core::Array<uint8_t>& out) X_FINAL
		{
			return core::Compression::LZMA::inflate(data, out);
		}
	};


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

	std::string inFile;
	std::string outFile;


 
    return 0;
}
