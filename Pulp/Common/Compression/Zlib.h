#pragma once


#ifndef X_COMPRESSION_ZLIB_H_
#define X_COMPRESSION_ZLIB_H_

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
	public:
		Zlib();
		~Zlib();


		// none buffed single step inflate / deflate.
		static size_t requiredDeflateDestBuf(size_t sourceLen);

		static bool deflate(const void* pSrcBuf, size_t srcBufLen, 
			void* pDstBuf, size_t destBufLen, size_t& destLenOut, CompressLevel::Enum lvl = CompressLevel::NORMAL);

		static bool inflate(void* pSrcBuf, size_t srcBufLen,
			void* pDstBuf, size_t destBufLen, CompressLevel::Enum lvl = CompressLevel::NORMAL);


	private:
	//	z_stream stream;

	};



} // namespace Compression

X_NAMESPACE_END

#endif // !X_COMPRESSION_ZLIB_H_