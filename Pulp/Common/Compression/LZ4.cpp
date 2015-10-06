#include <EngineCommon.h>
#include "LZ4.h"

#include <../../3rdparty/source/lz4-r131/lz4_lib.h>

X_NAMESPACE_BEGIN(core)

namespace Compression
{
	namespace
	{
		int compressLevelToAcceleration(LZ4::CompressLevel::Enum lvl)
		{
			if (lvl == LZ4::CompressLevel::LOW) {
				return 8;
			}
			if (lvl == LZ4::CompressLevel::NORMAL) {
				return 3;
			}
			if (lvl == LZ4::CompressLevel::HIGH) {
				return 1;
			}

			X_ASSERT_UNREACHABLE();
			return 1;
		}
	}

	size_t LZ4::maxSourceSize(void)
	{
		return static_cast<size_t>(LZ4_MAX_INPUT_SIZE);
	}

	size_t LZ4::requiredDeflateDestBuf(size_t sourceLen)
	{
		return static_cast<size_t>(LZ4_compressBound(safe_static_cast<int,size_t>(sourceLen)));
	}

	bool LZ4::deflate(const void* pSrcBuf, size_t srcBufLen,
		void* pDstBuf, size_t destBufLen, size_t& destLenOut, CompressLevel::Enum lvl)
	{
		const char* pSrc = reinterpret_cast<const char*>(pSrcBuf);
		char* pDst = reinterpret_cast<char*>(pDstBuf);

		int srcSize = safe_static_cast<int, size_t>(srcBufLen);
		int detSize = safe_static_cast<int, size_t>(destBufLen);

		int res = LZ4_compress_fast(pSrc, pDst, srcSize, detSize, 
			compressLevelToAcceleration(lvl));

		if (res <= 0) {
			X_ERROR("LZ4", "Failed to compress buffer: %i", res);
			destLenOut = 0;
			return false;
		}

		destLenOut = res;
		return true;
	}

	bool LZ4::inflate(void* pSrcBuf, size_t srcBufLen,
		void* pDstBuf, size_t destBufLen)
	{
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

	

} // namespace Compression

X_NAMESPACE_END