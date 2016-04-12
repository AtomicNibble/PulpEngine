#include <EngineCommon.h>
#include "Lzma2.h"

#include <../../3rdparty/source/lzma1506/LzmaLib.h>
#include <../../3rdparty/source/lzma1506/C/LzmaEnc.h>
#include <../../3rdparty/source/lzma1506/C/LzmaDec.h>

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
		// 0 <= level <= 9, default = 5
		int32_t compressLevelToLevel(LZMA::CompressLevel::Enum lvl)
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

		// 0 - fast, 1 - normal, default = 1 
		int32_t compressLevelToAlgo(LZMA::CompressLevel::Enum lvl)
		{
			if (lvl == LZMA::CompressLevel::LOW) {
				return 0;
			}
			if (lvl == LZMA::CompressLevel::NORMAL) {
				return 1;
			}
			if (lvl == LZMA::CompressLevel::HIGH) {
				return 1;
			}

			X_ASSERT_UNREACHABLE();
			return 1;
		}
		
		int32_t compressLevelToDictSize(LZMA::CompressLevel::Enum lvl)
		{
			if (lvl == LZMA::CompressLevel::LOW) {
				return 1 << 14; // 16kb
			}
			if (lvl == LZMA::CompressLevel::NORMAL) {
				return 1 << 16; // 64kb
			}
			if (lvl == LZMA::CompressLevel::HIGH) {
				return 1 << 20; // 1024kb
			}

			X_ASSERT_UNREACHABLE();
			return 1;
		}

		struct ArenaAlloc : public ISzAlloc
		{
			ArenaAlloc(core::MemoryArenaBase* arena) :
				arena_(arena)
			{
				X_ASSERT_NOT_NULL(arena);
				this->Alloc = allocLam;
				this->Free = freeLam;
			}

		private:
			static void* allocLam(void *p, size_t size)
			{
				ArenaAlloc* pThis = reinterpret_cast<ArenaAlloc*>(p);
				return X_NEW_ARRAY(uint8_t, size, pThis->arena_, "LzmaData");
			}
			static void freeLam(void *p, void *address) {
				ArenaAlloc* pThis = reinterpret_cast<ArenaAlloc*>(p);
				uint8_t* pData = reinterpret_cast<uint8_t*>(address);
				X_DELETE_ARRAY(pData, pThis->arena_);
			}

		private:
			core::MemoryArenaBase* arena_;
		};

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
	bool LZMA::deflate(core::MemoryArenaBase* arena, const void* pSrcBuf, size_t srcBufLen,
		void* pDstBuf, size_t destBufLen, size_t& destLenOut, CompressLevel::Enum lvl)
	{	
		const uint8_t* pSrc = reinterpret_cast<const uint8_t*>(pSrcBuf);
		uint8_t* pDst = reinterpret_cast<uint8_t*>(pDstBuf);

		size_t propsSize = LZMA_PROPS_SIZE;
		size_t destLenTemp = destBufLen;


		CLzmaEncProps props;
		LzmaEncProps_Init(&props);
		props.level = compressLevelToLevel(lvl);
		props.algo = compressLevelToAlgo(lvl);
		props.dictSize = compressLevelToDictSize(lvl);
		props.writeEndMark = 1; // 0 or 1
		props.numThreads = 1;

		LzmaEncProps_Normalize(&props);

		ArenaAlloc allocForLzma(arena);

		int res = LzmaEncode(
			&pDst[propsSize], &destLenTemp,
			pSrc, srcBufLen,
			&props, pDst, &propsSize, 1,
			nullptr,
			&allocForLzma,
			&allocForLzma);


		if (res == SZ_OK) {
			destLenOut = destLenTemp + propsSize;
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

	bool LZMA::inflate(core::MemoryArenaBase* arena, const void* pSrcBuf, size_t srcBufLen,
		void* pDstBuf, size_t destBufLen)
	{
		const uint8_t* pSrc = reinterpret_cast<const uint8_t*>(pSrcBuf);
		uint8_t* pDst = reinterpret_cast<uint8_t*>(pDstBuf);

		size_t propsSize = LZMA_PROPS_SIZE;
		size_t destLenTemp = destBufLen;
		size_t srcLenTemp = srcBufLen - propsSize;

		ArenaAlloc allocForLzma(arena);
		ELzmaStatus status;

		int res = LzmaDecode(pDst, &destLenTemp,
			&pSrc[propsSize], &srcLenTemp,
			pSrc, LZMA_PROPS_SIZE,
			LZMA_FINISH_ANY,
			&status,
			&allocForLzma
		);

		if (res == SZ_OK) {
			// status is one of following:
			// LZMA_STATUS_FINISHED_WITH_MARK
			// LZMA_STATUS_NOT_FINISHED
			// LZMA_STATUS_MAYBE_FINISHED_WITHOUT_MARK
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