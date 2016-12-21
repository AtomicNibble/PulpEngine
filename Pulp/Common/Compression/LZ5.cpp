#include <EngineCommon.h>
#include "LZ5.h"

#include <../../3rdparty/source/lz5/lz5_lib.h>
#include <../../3rdparty/source/lz5/lz5hc.h>


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

		int compressLevelToAccelerationHC(CompressLevel::Enum lvl)
		{
			if (lvl == CompressLevel::LOW) {
				return 4;
			}
			if (lvl == CompressLevel::NORMAL) {
				return 8;
			}
			if (lvl == CompressLevel::HIGH) {
				return 16;
			}

			X_ASSERT_UNREACHABLE();
			return 1;
		}

	} // namespace

	Algo::Enum LZ5::getAlgo(void)
	{
		return Algo::LZ5;
	}

	size_t LZ5::maxSourceSize(void)
	{
		return static_cast<size_t>(LZ5_MAX_INPUT_SIZE);
	}

	size_t LZ5::requiredDeflateDestBuf(size_t sourceLen)
	{
		return static_cast<size_t>(LZ5_compressBound(safe_static_cast<int, size_t>(sourceLen)));
	}

	bool LZ5::deflate(core::MemoryArenaBase* arena, const void* pSrcBuf, size_t srcBufLen,
		void* pDstBuf, size_t destBufLen, size_t& destLenOut, CompressLevel::Enum lvl)
	{
		X_UNUSED(arena);

		const char* pSrc = reinterpret_cast<const char*>(pSrcBuf);
		char* pDst = reinterpret_cast<char*>(pDstBuf);

		const int32_t srcSize = safe_static_cast<int, size_t>(srcBufLen);
		const int32_t detSize = safe_static_cast<int, size_t>(destBufLen);

		const int32_t res = LZ5_compress_fast(pSrc, pDst, srcSize, detSize,
			compressLevelToAcceleration(lvl));

		if (res <= 0) {
			X_ERROR("LZ5", "Failed to compress buffer: %" PRIi32, res);
			destLenOut = 0;
			return false;
		}

		destLenOut = res;
		return true;
	}

	bool LZ5::inflate(core::MemoryArenaBase* arena, const void* pSrcBuf, size_t srcBufLen,
		void* pDstBuf, size_t destBufLen)
	{
		X_UNUSED(arena);
		X_UNUSED(srcBufLen);

		const char* pSrc = reinterpret_cast<const char*>(pSrcBuf);
		char* pDst = reinterpret_cast<char*>(pDstBuf);
		const int32_t size = safe_static_cast<int, size_t>(destBufLen);

		const int32_t res = LZ5_decompress_fast(pSrc, pDst, size);

		if (res <= 0) {
			X_ERROR("LZ5", "Failed to decompress buffer: %" PRIi32, res);
			return false;
		}
		return true;
	}

	// ---------------------------------------


	bool LZ5HC::deflate(core::MemoryArenaBase* arena, const void* pSrcBuf, size_t srcBufLen,
		void* pDstBuf, size_t destBufLen, size_t& destLenOut, CompressLevel::Enum lvl)
	{
		X_UNUSED(arena);

		const char* pSrc = reinterpret_cast<const char*>(pSrcBuf);
		char* pDst = reinterpret_cast<char*>(pDstBuf);

		const int32_t srcSize = safe_static_cast<int, size_t>(srcBufLen);
		const int32_t detSize = safe_static_cast<int, size_t>(destBufLen);

		const int32_t res = LZ5_compress_HC(pSrc, pDst, srcSize, detSize, compressLevelToAccelerationHC(lvl));

		if (res <= 0) {
			X_ERROR("LZ5", "Failed to compress buffer: %" PRIi32, res);
			destLenOut = 0;
			return false;
		}

		destLenOut = res;
		return true;
	}


	// ---------------------------------------

	// check the helpers are correct.

	static_assert(LZ5Stream::maxSourceSize() == LZ5_MAX_INPUT_SIZE, "LZ5 max source size helper don't match lib value");
	static_assert(LZ5Stream::requiredDeflateDestBuf(0x1223345) == LZ5_COMPRESSBOUND(0x1223345), "LZ5 compressbound helper don't match lib result");


	LZ5Stream::LZ5Stream(core::MemoryArenaBase* arena) :
		arena_(arena),
		stream_(nullptr)
	{
		LZ5_stream_t* pStream = X_NEW(LZ5_stream_t, arena, "LZ5Stream");
		LZ5_resetStream(pStream);

		stream_ = pStream;
	}

	LZ5Stream::~LZ5Stream()
	{
		if (stream_) {
			X_DELETE(stream_, arena_);
		}
	}

	size_t LZ5Stream::compressContinue(const void* pSrcBuf, size_t srcBufLen, void* pDstBuf, size_t destBufLen,
		CompressLevel::Enum lvl)
	{
		X_ASSERT_NOT_NULL(stream_);
		LZ5_stream_t* pStream = reinterpret_cast<LZ5_stream_t*>(stream_);

		const char* pSrc = reinterpret_cast<const char*>(pSrcBuf);
		char* pDst = reinterpret_cast<char*>(pDstBuf);
		int srcSize = safe_static_cast<int, size_t>(srcBufLen);
		int dstSize = safe_static_cast<int, size_t>(destBufLen);

		int res = LZ5_compress_fast_continue(pStream, pSrc, pDst, srcSize, dstSize,
			compressLevelToAcceleration(lvl));

		if (res == 0) {
			X_ERROR("LZ5", "Failed to compress buffer");
		}

		return res;
	}

	// ---------------------------------------


	LZ5StreamDecode::LZ5StreamDecode()
	{
		static_assert(sizeof(decodeStream_) == sizeof(LZ5_streamDecode_t), "");
		LZ5_streamDecode_t* pStream = reinterpret_cast<LZ5_streamDecode_t*>(decodeStream_);

		LZ5_setStreamDecode(pStream, nullptr, 0);
	}

	LZ5StreamDecode::~LZ5StreamDecode()
	{
#if X_DEBUG
		core::zero_object(decodeStream_);
#endif // !X_DEBUG
	}


	size_t LZ5StreamDecode::decompressContinue(const void* pSrcBuf, void* pDstBuf, size_t originalSize)
	{
		LZ5_streamDecode_t* pStream = reinterpret_cast<LZ5_streamDecode_t*>(decodeStream_);

		const char* pSrc = reinterpret_cast<const char*>(pSrcBuf);
		char* pDst = reinterpret_cast<char*>(pDstBuf);
		int origSize = safe_static_cast<int, size_t>(originalSize);


		int res = LZ5_decompress_fast_continue(pStream, pSrc, pDst, origSize);

		if (res != origSize) {
			X_ERROR("LZ5", "Failed to decompress buffer");
		}

		return res;
	}

	size_t LZ5StreamDecode::decompressContinue(const void* pSrcBuf, void* pDstBuf, size_t compressedSize, size_t maxDecompressedSize)
	{
		LZ5_streamDecode_t* pStream = reinterpret_cast<LZ5_streamDecode_t*>(decodeStream_);

		const char* pSrc = reinterpret_cast<const char*>(pSrcBuf);
		char* pDst = reinterpret_cast<char*>(pDstBuf);
		int cmpSize = safe_static_cast<int, size_t>(compressedSize);
		int maxDecSize = safe_static_cast<int, size_t>(maxDecompressedSize);


		int res = LZ5_decompress_safe_continue(pStream, pSrc, pDst, cmpSize, maxDecSize);


		return res;
	}


} // namespace Compression

X_NAMESPACE_END