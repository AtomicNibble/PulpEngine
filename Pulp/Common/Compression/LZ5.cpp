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

	Results from i7 3930k:
			Compressor name         Compress. Decompress. Compr. size  Ratio Filename
			memcpy                   7344 MB/s  7579 MB/s   211947520 100.00 silesia.tar
			
			lizard 1.0 -10            388 MB/s  3147 MB/s   103402971  48.79 silesia.tar
			lizard 1.0 -12            116 MB/s  2946 MB/s    86232422  40.69 silesia.tar
			lizard 1.0 -15             57 MB/s  3070 MB/s    81187330  38.31 silesia.tar
			lizard 1.0 -19           3.39 MB/s  2974 MB/s    77416400  36.53 silesia.tar
			
			lizard 1.0 -20            318 MB/s  2033 MB/s    96924204  45.73 silesia.tar
			lizard 1.0 -22            130 MB/s  2057 MB/s    84866725  40.04 silesia.tar
			lizard 1.0 -25             16 MB/s  2066 MB/s    75161667  35.46 silesia.tar
			lizard 1.0 -29           1.54 MB/s  2059 MB/s    68694227  32.41 silesia.tar
			
			lizard 1.0 -30            279 MB/s  1012 MB/s    85727429  40.45 silesia.tar
			lizard 1.0 -32            120 MB/s  1135 MB/s    78652654  37.11 silesia.tar
			lizard 1.0 -35             61 MB/s  1590 MB/s    74563583  35.18 silesia.tar
			lizard 1.0 -39           3.33 MB/s  1728 MB/s    69807522  32.94 silesia.tar

			lizard 1.0 -40            231 MB/s  1027 MB/s    80843049  38.14 silesia.tar
			lizard 1.0 -42            111 MB/s  1124 MB/s    73350988  34.61 silesia.tar
			lizard 1.0 -45             15 MB/s  1286 MB/s    66692694  31.47 silesia.tar
			lizard 1.0 -49           1.55 MB/s  1293 MB/s    60679215  28.63 silesia.tar

			lz4 1.7.5                 527 MB/s  2550 MB/s   100880800  47.60 silesia.tar
	(lz4fast is the unsafe lz4 functions.)
			lz4fast 1.7.5 -3          625 MB/s  2583 MB/s   107066190  50.52 silesia.tar
			lz4fast 1.7.5 -17         967 MB/s  3059 MB/s   131732802  62.15 silesia.tar
			lz4hc 1.7.5 -1            118 MB/s  2326 MB/s    87591763  41.33 silesia.tar
			lz4hc 1.7.5 -4             67 MB/s  2472 MB/s    79807909  37.65 silesia.tar
			lz4hc 1.7.5 -9             28 MB/s  2539 MB/s    77892285  36.75 silesia.tar
			lz4hc 1.7.5 -12          4.08 MB/s  2558 MB/s    77268977  36.46 silesia.tar

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
				return 30;
			}
			if (lvl == CompressLevel::NORMAL) {
				return 35;
			}
			if (lvl == CompressLevel::HIGH) {
				return 39;
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
		stream_(nullptr)
	{
		X_UNUSED(arena);

		LZ5_stream_t* pStream = LZ5_createStream(compressLevelToAcceleration(lvl));

		stream_ = pStream;
	}

	LZ5Stream::~LZ5Stream()
	{
		if (stream_) {
			LZ5_freeStream(static_cast<LZ5_stream_t*>(stream_));
		}
	}

	bool LZ5Stream::loadDict(const uint8_t* pDict, size_t size)
	{
		if (size < sizeof(SharedDictHdr)) {
			X_ERROR("LZ4", "Dictionary is too small to be valid");
			return false;
		}

		const SharedDictHdr* pDictHdr = reinterpret_cast<const SharedDictHdr*>(pDict);
		if (!pDictHdr->IsMagicValid()) {
			X_ERROR("LZ4", "Dictionary header is not valid");
			return false;
		}

		const int32_t cappedSize = core::Min(safe_static_cast<int32_t>(size), 64 * 1024);

		int32_t dictSize = LZ5_loadDict(
			reinterpret_cast<LZ5_stream_t*>(stream_),
			reinterpret_cast<const char*>(pDict),
			cappedSize
		);

		if (dictSize == 0 || dictSize < cappedSize) {
			X_ERROR("LZ4", "Failed to set dictionary");
			return false;
		}

		return true;
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

	bool LZ5StreamDecode::loadDict(const uint8_t* pDict, size_t size)
	{
		if (size < sizeof(SharedDictHdr)) {
			X_ERROR("LZ4", "Dictionary is too small to be valid");
			return false;
		}

		const SharedDictHdr* pDictHdr = reinterpret_cast<const SharedDictHdr*>(pDict);
		if (!pDictHdr->IsMagicValid()) {
			X_ERROR("LZ4", "Dictionary header is not valid");
			return false;
		}

		const int32_t cappedSize = core::Min(safe_static_cast<int32_t>(size), 64 * 1024);

		int32_t res = LZ5_setStreamDecode(
			reinterpret_cast<LZ5_streamDecode_t*>(decodeStream_),
			reinterpret_cast<const char*>(pDict),
			cappedSize
		);

		return res == 1;
	}

	size_t LZ5StreamDecode::decompressContinue(const void* pSrcBuf, void* pDstBuf, size_t compressedSize, size_t maxDecompressedSize)
	{
		LZ5_streamDecode_t* pStream = reinterpret_cast<LZ5_streamDecode_t*>(decodeStream_);

		const char* pSrc = reinterpret_cast<const char*>(pSrcBuf);
		char* pDst = reinterpret_cast<char*>(pDstBuf);
		int cmpSize = safe_static_cast<int, size_t>(compressedSize);
		int maxDecSize = safe_static_cast<int, size_t>(maxDecompressedSize);

		int res = LZ5_decompress_safe_continue(pStream, pSrc, pDst, cmpSize, maxDecSize);

		if (res < 0) {
			X_ERROR("LZ5", "Error decompressing stream");
			return 0;
		}

		return res;
	}


} // namespace Compression

X_NAMESPACE_END