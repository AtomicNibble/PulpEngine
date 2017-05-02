#include <EngineCommon.h>
#include "LZ5.h"

#include <../../3rdparty/source/lz5/lz5_common.h>
#include <../../3rdparty/source/lz5/lz5_compress.h>
#include <../../3rdparty/source/lz5/lz5_decompress.h>




X_NAMESPACE_BEGIN(core)

namespace Compression
{
	namespace
	{
		/*
			lvl				Compression		Decompress.		Compr. size		Ratio
	(10-19) fastLZ4: 
			lz5 2.0 -10		346 MB/s		2610 MB/s		103402971		48.79
			lz5 2.0 -12		103 MB/s		2458 MB/s		86232422		40.69
			lz5 2.0 -15		50 MB/s			2552 MB/s		81187330		38.31
			lz5 2.0 -19		3.04 MB/s		2497 MB/s		77416400		36.53
	(20-29) LZ5v2: 
			lz5 2.0 -21		157 MB/s		1795 MB/s		89239174		42.10
			lz5 2.0 -23		30 MB/s			1778 MB/s		81097176		38.26
			lz5 2.0 -26		6.63 MB/s		1734 MB/s		74503695		35.15
			lz5 2.0 -29		1.37 MB/s		1634 MB/s		68694227		32.41
	(30-39) fastLZ4 + Huffman: 
			lz5 2.0 -30		246 MB/s		909 MB/s		85727429		40.45
			lz5 2.0 -32		94 MB/s			1244 MB/s		76929454		36.30
			lz5 2.0 -35		47 MB/s			1435 MB/s		73850400		34.84
			lz5 2.0 -39		2.94 MB/s		1502 MB/s		69807522		32.94
	(40-49) LZ5v2 + Huffman: 	
			lz5 2.0 -41		126 MB/s		961 MB/s		76100661		35.91
			lz5 2.0 -43		28 MB/s			1101 MB/s		70955653		33.48
			lz5 2.0 -46		6.25 MB/s		1073 MB/s		65413061		30.86
			lz5 2.0 -49		1.27 MB/s		1064 MB/s		60679215		28.63
		*/

		int compressLevelToAcceleration(CompressLevel::Enum lvl)
		{
			if (lvl == CompressLevel::LOW) {
				return LZ5_MIN_CLEVEL;
			}
			if (lvl == CompressLevel::NORMAL) {
				return 12;
			}
			if (lvl == CompressLevel::HIGH) {
				return 19;
			}

			X_ASSERT_UNREACHABLE();
			return LZ5_MIN_CLEVEL;
		}

		int compressLevelToAccelerationHC(CompressLevel::Enum lvl)
		{
			if (lvl == CompressLevel::LOW) {
				return 40;
			}
			if (lvl == CompressLevel::NORMAL) {
				return 45;
			}
			if (lvl == CompressLevel::HIGH) {
				return LZ5_MAX_CLEVEL;
			}

			X_ASSERT_UNREACHABLE();
			return LZ5_MIN_CLEVEL;
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

		const int32_t res = LZ5_compress(pSrc, pDst, srcSize, detSize,
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

		const char* pSrc = reinterpret_cast<const char*>(pSrcBuf);
		char* pDst = reinterpret_cast<char*>(pDstBuf);
		const int32_t srcSize = safe_static_cast<int, size_t>(srcBufLen);
		const int32_t detSize = safe_static_cast<int, size_t>(destBufLen);

		const int32_t res = LZ5_decompress_safe(pSrc, pDst, srcSize, detSize);

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

		const int32_t res = LZ5_compress(pSrc, pDst, srcSize, detSize, compressLevelToAccelerationHC(lvl));

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


	LZ5Stream::LZ5Stream(core::MemoryArenaBase* arena, CompressLevel::Enum lvl) :
		arena_(arena),
		stream_(nullptr)
	{
		LZ5_stream_t* pStream = X_NEW(LZ5_stream_t, arena, "LZ5Stream");
		LZ5_resetStream(pStream, compressLevelToAcceleration(lvl));

		stream_ = pStream;
	}

	LZ5Stream::~LZ5Stream()
	{
		if (stream_) {
			X_DELETE(stream_, arena_);
		}
	}

	size_t LZ5Stream::compressContinue(const void* pSrcBuf, size_t srcBufLen, void* pDstBuf, size_t destBufLen)
	{
		X_ASSERT_NOT_NULL(stream_);
		LZ5_stream_t* pStream = reinterpret_cast<LZ5_stream_t*>(stream_);

		const char* pSrc = reinterpret_cast<const char*>(pSrcBuf);
		char* pDst = reinterpret_cast<char*>(pDstBuf);
		int srcSize = safe_static_cast<int, size_t>(srcBufLen);
		int dstSize = safe_static_cast<int, size_t>(destBufLen);

		int res = LZ5_compress_continue(pStream, pSrc, pDst, srcSize, dstSize);

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