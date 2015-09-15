#include <EngineCommon.h>
#include "Lzma2.h"

#include <../../3rdparty/source/lzma1506/LzmaLib.h>

#if X_DEBUG
X_LINK_LIB("LzmaLibd");
#else
X_LINK_LIB("LzmaLib");
#endif // !X_DEBUG


X_NAMESPACE_BEGIN(core)

namespace Compression
{
	namespace
	{
		int compressLevelToInt(LZMA::CompressLevel::Enum lvl)
		{
			if (lvl == LZMA::CompressLevel::LOW) {
				return 1;
			}
			if (lvl == LZMA::CompressLevel::NORMAL) {
				return 5;
			}
			if (lvl == LZMA::CompressLevel::HIGH) {
				return 7;
			}

			X_ASSERT_UNREACHABLE();
			return 1;
		}
	}

	size_t LZMA::maxSourceSize(void)
	{
		// dunno what limit is.
		return std::numeric_limits<size_t>::max();
	}

	// buffer than source is garanted to fit into.
	size_t LZMA::requiredDeflateDestBuf(size_t sourceLen)
	{
		return (sourceLen + sourceLen / 3 + 128) + LZMA_PROPS_SIZE;
	}

	// none buffed single step inflate / deflate.
	bool LZMA::deflate(const void* pSrcBuf, size_t srcBufLen,
		void* pDstBuf, size_t destBufLen, size_t& destLenOut, CompressLevel::Enum lvl)
	{	
		const uint8_t* pSrc = reinterpret_cast<const uint8_t*>(pSrcBuf);
		uint8_t* pDst = reinterpret_cast<uint8_t*>(pDstBuf);

		size_t propsSize = LZMA_PROPS_SIZE;
		size_t destLenTemp = destBufLen;

		int res = LzmaCompress(
			&pDst[propsSize], &destLenTemp,
			pSrc, srcBufLen,
			pDst, &propsSize, 
			compressLevelToInt(lvl),
			0,   // dictSize (0 == default)
			-1,  // lc
			-1,  // lp
			-1,  // pb
			-1,  // fb
			1    // numthreads
			);

		if (res == SZ_OK) {
			destLenOut = destLenTemp;
			return true;
		}

		destLenOut = 0;

		// error
		if (res == SZ_ERROR_DATA) {
			X_ERROR("LZMA","Data error during compression");
		}
		else if (res == SZ_ERROR_MEM) {
			X_ERROR("LZMA", "Memory allocation error occured during compression");
		}
		else if (res == SZ_ERROR_UNSUPPORTED) {
			X_ERROR("LZMA", "Unsupported properties when compressing");
		}
		else if (res == SZ_ERROR_INPUT_EOF) {
			X_ERROR("LZMA", "Require more bytes in input buffer when compressing");
		}
		return false;
	}

	bool LZMA::inflate(void* pSrcBuf, size_t srcBufLen,
		void* pDstBuf, size_t destBufLen)
	{
		const uint8_t* pSrc = reinterpret_cast<const uint8_t*>(pSrcBuf);
		uint8_t* pDst = reinterpret_cast<uint8_t*>(pDstBuf);

		size_t propsSize = LZMA_PROPS_SIZE;
		size_t destLenTemp = destBufLen;
		size_t srcLenTemp = srcBufLen - propsSize;

		int res = LzmaUncompress(
			pDst, &destLenTemp,
			&pSrc[propsSize], &srcLenTemp,
			pSrc, propsSize
		);

		if (res == SZ_OK) {
			return true;
		}

		// error
		if (res == SZ_ERROR_DATA) {
			X_ERROR("LZMA", "Data error during decompression");
		}
		else if (res == SZ_ERROR_MEM) {
			X_ERROR("LZMA", "Memory allocation error occured during decompression");
		}
		else if (res == SZ_ERROR_UNSUPPORTED) {
			X_ERROR("LZMA", "Unsupported properties when decompression");
		}
		else if (res == SZ_ERROR_INPUT_EOF) {
			X_ERROR("LZMA", "Require more bytes in input buffer when decompression");
		}
		return false;
	}



} // namespace Compression

X_NAMESPACE_END