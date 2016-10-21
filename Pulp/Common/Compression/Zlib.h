#pragma once

#include <functional>

#ifndef X_COMPRESSION_ZLIB_H_
#define X_COMPRESSION_ZLIB_H_

extern "C" {
	struct z_stream_s;
};

#include <CompileTime\IsPOD.h>
#include <Containers\Array.h>
#include <ICompression.h>

X_NAMESPACE_BEGIN(core)

namespace Compression
{
	///
	///   Zlib decompressor(CompressBuf);
	///
	///
	///
	///

	class Zlib
	{
	public:
		static Algo::Enum getAlgo(void);

		static size_t maxSourceSize(void);

		// none buffed single step inflate / deflate.
		static size_t requiredDeflateDestBuf(size_t sourceLen);

		static bool deflate(core::MemoryArenaBase* arena, const void* pSrcBuf, size_t srcBufLen,
			void* pDstBuf, size_t destBufLen, size_t& destLenOut, CompressLevel::Enum lvl = CompressLevel::NORMAL);

		static bool inflate(core::MemoryArenaBase* arena, const void* pSrcBuf, size_t srcBufLen,
			void* pDstBuf, size_t destBufLen);


		template<typename T>
		static bool deflate(core::MemoryArenaBase* arena, const core::Array<T>& data,
			core::Array<uint8_t>& compressed,
			CompressLevel::Enum lvl = CompressLevel::NORMAL);

		template<typename T>
		static bool inflate(core::MemoryArenaBase* arena, const core::Array<uint8_t>& data,
			core::Array<T>& inflated);


	private:
		X_NO_CREATE(Zlib);
		X_NO_COPY(Zlib);
		X_NO_ASSIGN(Zlib);
	};

	template<typename T>
	X_INLINE bool Zlib::deflate(core::MemoryArenaBase* arena, const core::Array<T>& data, core::Array<uint8_t>& compressed,
		CompressLevel::Enum lvl)
	{
		static_assert(compileTime::IsPOD<T>::Value, "T must be a POD type.");

		size_t compressedSize = 0;
		size_t bufSize = requiredDeflateDestBuf(data.size() * sizeof(T));

		compressed.resize(bufSize);

		bool res = deflate(arena, data.ptr(), data.size() * sizeof(T),
			compressed.ptr(), compressed.size(), compressedSize, lvl);

		compressed.resize(compressedSize);
		return res;
	}

	template<typename T>
	X_INLINE bool Zlib::inflate(core::MemoryArenaBase* arena, const core::Array<uint8_t>& data, core::Array<T>& inflated)
	{
		static_assert(compileTime::IsPOD<T>::Value, "T must be a POD type.");

		return inflate(arena, data.ptr(), data.size(), inflated.ptr(), inflated.size() * sizeof(T));
	}


	// can take one or many inputs and inflate them into dest.
	class ZlibInflate
	{
	public:
		X_DECLARE_ENUM(InflateResult)(ERROR,OK,DONE);

	public:
		ZlibInflate(core::MemoryArenaBase* arena, void* pDst, size_t destLen);
		~ZlibInflate();

		InflateResult::Enum Inflate(const void* pCompessedData, size_t len);

	private:
		X_NO_COPY(ZlibInflate);
		X_NO_ASSIGN(ZlibInflate);
	private:
		core::MemoryArenaBase* arena_;
		z_stream_s* stream_;

		const void* pDst_;
		size_t destLen_;
	};

	// can compress into blocks.
	class ZlibDefalte
	{
	public:
		X_DECLARE_ENUM(DeflateResult)(ERROR, OK);

		static const size_t DEFAULT_BUF_SIZE = 1024 * 16;

		typedef std::function<void(const uint8_t* pData, size_t len)> DeflateCallback;

	public:
		ZlibDefalte(core::MemoryArenaBase* arena, DeflateCallback defalteCallBack, CompressLevel::Enum lvl = CompressLevel::NORMAL);
		~ZlibDefalte();

		void setBufferSize(size_t size);

		// this will accept N input blocks and call the deflate callback everytime the buffer is full or we are flusing (finish == true)
		// you can just pass a huge src block with finish == true and the callback will keep been called with blocks untill it's finished.
		DeflateResult::Enum Deflate(const void* pSrcData, size_t len, bool finish);

	private:
		X_NO_COPY(ZlibDefalte);
		X_NO_ASSIGN(ZlibDefalte);

	private:
		DeflateCallback callback_;

	private:
		core::MemoryArenaBase* arena_;
		core::Array<uint8_t> buffer_;
		z_stream_s* stream_;
	};

} // namespace Compression

X_NAMESPACE_END

#endif // !X_COMPRESSION_ZLIB_H_