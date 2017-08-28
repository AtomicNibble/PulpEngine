#include <EngineCommon.h>
#include "Store.h"



X_NAMESPACE_BEGIN(core)

namespace Compression
{

	Algo::Enum Store::getAlgo(void)
	{
		return Algo::STORE;
	}

	size_t Store::maxSourceSize(void)
	{
		return std::numeric_limits<size_t>::max();
	}

	size_t Store::requiredDeflateDestBuf(size_t sourceLen)
	{
		return sourceLen;
	}

	bool Store::deflate(core::MemoryArenaBase* arena, const void* pSrcBuf, size_t srcBufLen,
		void* pDstBuf, size_t destBufLen, size_t& destLenOut, CompressLevel::Enum lvl)
	{
		X_UNUSED(arena);
		X_UNUSED(lvl);

		if (destBufLen != srcBufLen) {
			X_ERROR("Store", "Dest buf size does not match src. srcLen: %" PRIuS " destLen: %" PRIuS, srcBufLen, destBufLen);
			return false;
		}

		std::memcpy(pDstBuf, pSrcBuf, destBufLen);

		destLenOut = destBufLen;
		return true;
	}

	bool Store::inflate(core::MemoryArenaBase* arena, const void* pSrcBuf, size_t srcBufLen,
		void* pDstBuf, size_t destBufLen)
	{
		X_UNUSED(arena);

		if (destBufLen != srcBufLen) {
			X_ERROR("Store", "Dest buf size does not match src. srcLen: %" PRIuS " destLen: %" PRIuS, srcBufLen, destBufLen);
			return false;
		}

		std::memcpy(pDstBuf, pSrcBuf, destBufLen);
		return true;
	}

	// ---------------------------------------


}


X_NAMESPACE_END