#pragma once


#ifndef X_COMPRESSION_ZLIB_H_
#define X_COMPRESSION_ZLIB_H_

extern "C" {
	struct z_stream_s;
};

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

		static bool inflate(void* pSrcBuf, size_t srcBufLen,
			void* pDstBuf, size_t destBufLen);


	private:
		X_NO_COPY(Zlib);
		X_NO_ASSIGN(Zlib);
	protected:
		z_stream_s* stream_;
	};


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
		const void* pDst_;
		size_t destLen_;
	};


} // namespace Compression

X_NAMESPACE_END

#endif // !X_COMPRESSION_ZLIB_H_