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
		// 0 <= level <= 9, default = 5
		int32_t compressLevelToLevel(CompressLevel::Enum lvl)
		{
			if (lvl == CompressLevel::LOW) {
				return 1;
			}
			if (lvl == CompressLevel::NORMAL) {
				return 5;
			}
			if (lvl == CompressLevel::HIGH) {
				return 7;
			}

			X_ASSERT_UNREACHABLE();
			return 1;
		}

		// 0 - fast, 1 - normal, default = 1 
		int32_t compressLevelToAlgo(CompressLevel::Enum lvl)
		{
			if (lvl == CompressLevel::LOW) {
				return 0;
			}
			if (lvl == CompressLevel::NORMAL) {
				return 1;
			}
			if (lvl == CompressLevel::HIGH) {
				return 1;
			}

			X_ASSERT_UNREACHABLE();
			return 1;
		}
		
		int32_t compressLevelToDictSize(CompressLevel::Enum lvl)
		{
			if (lvl == CompressLevel::LOW) {
				return 1024 * 16; // 16kb
			}
			if (lvl == CompressLevel::NORMAL) {
				return 1024 * 1024; // 1MB
			}
			if (lvl == CompressLevel::HIGH) {
				return 1024 * 1024 * 16; // 16MB
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
			static void* __fastcall allocLam(void *p, size_t size)
			{
				ArenaAlloc* pThis = reinterpret_cast<ArenaAlloc*>(p);
				return X_NEW_ARRAY(uint8_t, size, pThis->arena_, "LzmaData");
			}
			static void __fastcall freeLam(void *p, void *address) {
				ArenaAlloc* pThis = reinterpret_cast<ArenaAlloc*>(p);
				uint8_t* pData = reinterpret_cast<uint8_t*>(address);
				X_DELETE_ARRAY(pData, pThis->arena_);
			}

		private:
			core::MemoryArenaBase* arena_;
		};

	}

	Algo::Enum LZMA::getAlgo(void)
	{
		return Algo::LZMA;
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

		ArenaAlloc allocForLzma(arena);

		int res = LzmaCompress(
			&pDst[propsSize], &destLenTemp,
			pSrc, srcBufLen,
			pDst, &propsSize,
			compressLevelToLevel(lvl),
			compressLevelToAlgo(lvl),
			compressLevelToDictSize(lvl),
			-1,  // lc
			-1,  // lp
			-1,  // pb
			-1,  // fb
			1,    // numthreads
			&allocForLzma,
			&allocForLzma
		);

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

		int res = LzmaUncompress(
			pDst, &destLenTemp,
			&pSrc[propsSize], &srcLenTemp,
			pSrc, propsSize,
			&allocForLzma
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