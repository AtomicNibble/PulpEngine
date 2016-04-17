#pragma once


#ifndef X_COMPRESSION_LZMA2_H_
#define X_COMPRESSION_LZMA2_H_

#include <Containers\Array.h>
#include <ICompression.h>

X_NAMESPACE_BEGIN(core)

namespace Compression
{

	class LZMA
	{
	public:
		static Algo::Enum getAlgo(void);

		// max source buffer size.
		static size_t maxSourceSize(void);
		// buffer than source is garanted to fit into.
		static size_t requiredDeflateDestBuf(size_t sourceLen);

		// none buffed single step inflate / deflate.
		static bool deflate(core::MemoryArenaBase* arena, const void* pSrcBuf, size_t srcBufLen,
			void* pDstBuf, size_t destBufLen, size_t& destLenOut, CompressLevel::Enum lvl = CompressLevel::NORMAL);

		static bool inflate(core::MemoryArenaBase* arena, const void* pSrcBuf, size_t srcBufLen,
			void* pDstBuf, size_t destBufLen);

		template<typename T>
		static bool deflate(core::MemoryArenaBase* arena, const core::Array<T>& data,
			core::Array<uint8_t>& compressed,
			CompressLevel::Enum lvl = CompressLevel::NORMAL);

		template<typename T>
		static bool inflate(core::MemoryArenaBase* arena, const core::Array<T>& data,
			core::Array<uint8_t>& inflated);

	private:
		X_NO_CREATE(LZMA);
		X_NO_COPY(LZMA);
		X_NO_ASSIGN(LZMA);
	};


	template<typename T>
	X_INLINE bool LZMA::deflate(core::MemoryArenaBase* arena, const core::Array<T>& data, core::Array<uint8_t>& compressed,
		CompressLevel::Enum lvl)
	{
		size_t compressedSize = 0;
		size_t bufSize = requiredDeflateDestBuf(data.size());

		compressed.resize(bufSize);

		bool res = deflate(arena, data.ptr(), data.size(),
			compressed.ptr(), compressed.size(), compressedSize, lvl);

		compressed.resize(compressedSize);
		return res;
	}

	template<typename T>
	X_INLINE bool LZMA::inflate(core::MemoryArenaBase* arena, const core::Array<T>& data, core::Array<uint8_t>& inflated)
	{
		return inflate(arena, data.ptr(), data.size(), inflated.ptr(), inflated.size());
	}


} // namespace Compression

X_NAMESPACE_END

#endif // !X_COMPRESSION_LZMA2_H_