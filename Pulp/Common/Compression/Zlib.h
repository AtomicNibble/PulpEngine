#pragma once


#ifndef X_COMPRESSION_ZLIB_H_
#define X_COMPRESSION_ZLIB_H_

extern "C" {
	struct z_stream_s;
};

#include <Containers\Array.h>

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
		X_DECLARE_ENUM(CompressLevel)(
			LOW, // speed
			NORMAL, // normal
			HIGH // best
			);
	protected:
		Zlib();
		virtual ~Zlib();

	public:
		// none buffed single step inflate / deflate.
		static size_t requiredDeflateDestBuf(size_t sourceLen);

		static bool deflate(const void* pSrcBuf, size_t srcBufLen, 
			void* pDstBuf, size_t destBufLen, size_t& destLenOut, CompressLevel::Enum lvl = CompressLevel::NORMAL);

		static bool inflate(const void* pSrcBuf, size_t srcBufLen,
			void* pDstBuf, size_t destBufLen);


		template<typename T>
		static bool deflate(const core::Array<T>& data,
			core::Array<uint8_t>& compressed,
			CompressLevel::Enum lvl = CompressLevel::NORMAL);

		template<typename T>
		static bool inflate(const core::Array<T>& data,
			core::Array<uint8_t>& inflated);


	private:
		X_NO_COPY(Zlib);
		X_NO_ASSIGN(Zlib);
	protected:
		z_stream_s* stream_;
	};

	template<typename T>
	X_INLINE bool Zlib::deflate(const core::Array<T>& data, core::Array<uint8_t>& compressed,
		CompressLevel::Enum lvl)
	{
		size_t compressedSize = 0;
		size_t bufSize = requiredDeflateDestBuf(data.size());

		compressed.resize(bufSize);

		bool res = deflate(data.ptr(), data.size(),
			compressed.ptr(), compressed.size(), compressedSize, lvl);

		compressed.resize(compressedSize);
		return res;
	}

	template<typename T>
	X_INLINE bool Zlib::inflate(const core::Array<T>& data, core::Array<uint8_t>& inflated)
	{
		return inflate(data.ptr(), data.size(), inflated.ptr(), inflated.size());
	}


	// can take one or many inputs and inflate them into dest.
	struct ZlibInflate : public Zlib
	{
	public:
		X_DECLARE_ENUM(InflateResult)(ERROR,OK,DONE);
	public:
		ZlibInflate(void* pDst, size_t destLen);
		virtual ~ZlibInflate() X_FINAL;

		InflateResult::Enum Inflate(const void* pCompessedData, size_t len);

	private:
		X_NO_COPY(ZlibInflate);
		X_NO_ASSIGN(ZlibInflate);
	private:
		const void* pDst_;
		size_t destLen_;
	};


} // namespace Compression

X_NAMESPACE_END

#endif // !X_COMPRESSION_ZLIB_H_