#include <EngineCommon.h>
#include "LZ4.h"

#include <../../3rdparty/source/lz4-r131/lz4_lib.h>
#include <../../3rdparty/source/lz4-r131/lz4hc.h>

X_NAMESPACE_BEGIN(core)

namespace Compression
{
	namespace
	{
		int compressLevelToAcceleration(CompressLevel::Enum lvl)
		{
			if (lvl == CompressLevel::LOW) {
				return 8;
			}
			if (lvl == CompressLevel::NORMAL) {
				return 3;
			}
			if (lvl == CompressLevel::HIGH) {
				return 1;
			}

			X_ASSERT_UNREACHABLE();
			return 1;
		}
	} // namespace

	Algo::Enum LZ4::getAlgo(void)
	{
		return Algo::LZ4;
	}

	size_t LZ4::maxSourceSize(void)
	{
		return static_cast<size_t>(LZ4_MAX_INPUT_SIZE);
	}

	size_t LZ4::requiredDeflateDestBuf(size_t sourceLen)
	{
		return static_cast<size_t>(LZ4_compressBound(safe_static_cast<int,size_t>(sourceLen)));
	}

	bool LZ4::deflate(core::MemoryArenaBase* arena, const void* pSrcBuf, size_t srcBufLen,
		void* pDstBuf, size_t destBufLen, size_t& destLenOut, CompressLevel::Enum lvl)
	{
		X_UNUSED(arena);

		const char* pSrc = reinterpret_cast<const char*>(pSrcBuf);
		char* pDst = reinterpret_cast<char*>(pDstBuf);

		int srcSize = safe_static_cast<int, size_t>(srcBufLen);
		int detSize = safe_static_cast<int, size_t>(destBufLen);

		int res = 0;

		if (CompressLevel::HIGH == lvl)
		{
			res = LZ4_compress_HC(pSrc, pDst, srcSize, detSize, 16);
		}
		else
		{
			res = LZ4_compress_fast(pSrc, pDst, srcSize, detSize,
				compressLevelToAcceleration(lvl));
		}

		if (res <= 0) {
			X_ERROR("LZ4", "Failed to compress buffer: %i", res);
			destLenOut = 0;
			return false;
		}

		destLenOut = res;
		return true;
	}

	bool LZ4::inflate(core::MemoryArenaBase* arena, const void* pSrcBuf, size_t srcBufLen,
		void* pDstBuf, size_t destBufLen)
	{
		X_UNUSED(arena);
		X_UNUSED(srcBufLen);

		const char* pSrc = reinterpret_cast<const char*>(pSrcBuf);
		char* pDst = reinterpret_cast<char*>(pDstBuf);

		int size = safe_static_cast<int, size_t>(destBufLen);

		int res = LZ4_decompress_fast(pSrc, pDst, size);

		if (res <= 0) {
			X_ERROR("LZ4", "Failed to decompress buffer: %i", res);
			return false;
		}
		return true;
	}


	// ---------------------------------------

	// check the helpers are correct.

	static_assert(LZ4Stream::maxSourceSize() == LZ4_MAX_INPUT_SIZE, "LZ4 max source size helper don't match lib value");
	static_assert(LZ4Stream::requiredDeflateDestBuf(0x1223345) == LZ4_COMPRESSBOUND(0x1223345), "LZ4 compressbound helper don't match lib result");


	LZ4Stream::LZ4Stream(core::MemoryArenaBase* arena) : 
		arena_(arena),
		stream_(nullptr)
	{
		LZ4_stream_t* pStream = X_NEW(LZ4_stream_t, arena, "LZ4Stream");
		LZ4_resetStream(pStream);

		stream_ = pStream;
	}

	LZ4Stream::~LZ4Stream()
	{
		if (stream_) {
			X_DELETE(stream_, arena_);
		}
	}

	size_t LZ4Stream::compressContinue(const void* pSrcBuf, size_t srcBufLen, void* pDstBuf, size_t destBufLen,
		CompressLevel::Enum lvl)
	{
		X_ASSERT_NOT_NULL(stream_);
		LZ4_stream_t* pStream = reinterpret_cast<LZ4_stream_t*>(stream_);

		const char* pSrc = reinterpret_cast<const char*>(pSrcBuf);
		char* pDst = reinterpret_cast<char*>(pDstBuf);
		int srcSize = safe_static_cast<int, size_t>(srcBufLen);
		int dstSize = safe_static_cast<int, size_t>(destBufLen);

		int res = LZ4_compress_fast_continue(pStream, pSrc, pDst, srcSize, dstSize,
			compressLevelToAcceleration(lvl));

		if (res == 0) {
			X_ERROR("LZ4", "Failed to compress buffer");
		}

		return res;
	}

	// ---------------------------------------


	LZ4StreamDecode::LZ4StreamDecode()
	{
		static_assert(sizeof(decodeStream_) == sizeof(LZ4_streamDecode_t), "");
		LZ4_streamDecode_t* pStream = reinterpret_cast<LZ4_streamDecode_t*>(decodeStream_);

		LZ4_setStreamDecode(pStream, nullptr, 0);
	}

	LZ4StreamDecode::~LZ4StreamDecode()
	{
#if X_DEBUG
		core::zero_object(decodeStream_);
#endif // !X_DEBUG
	}


	size_t LZ4StreamDecode::decompressContinue(const void* pSrcBuf, void* pDstBuf, size_t originalSize)
	{
		LZ4_streamDecode_t* pStream = reinterpret_cast<LZ4_streamDecode_t*>(decodeStream_);

		const char* pSrc = reinterpret_cast<const char*>(pSrcBuf);
		char* pDst = reinterpret_cast<char*>(pDstBuf);
		int origSize = safe_static_cast<int, size_t>(originalSize);


		int res = LZ4_decompress_fast_continue(pStream, pSrc, pDst, origSize);

		if (res != origSize) {
			X_ERROR("LZ4", "Failed to decompress buffer");
		}

		return res;
	}

	size_t LZ4StreamDecode::decompressContinue(const void* pSrcBuf, void* pDstBuf, size_t compressedSize, size_t maxDecompressedSize)
	{
		LZ4_streamDecode_t* pStream = reinterpret_cast<LZ4_streamDecode_t*>(decodeStream_);

		const char* pSrc = reinterpret_cast<const char*>(pSrcBuf);
		char* pDst = reinterpret_cast<char*>(pDstBuf);
		int cmpSize = safe_static_cast<int, size_t>(compressedSize);
		int maxDecSize = safe_static_cast<int, size_t>(maxDecompressedSize);


		int res = LZ4_decompress_safe_continue(pStream, pSrc, pDst, cmpSize, maxDecSize);


		return res;
	}


} // namespace Compression

X_NAMESPACE_END