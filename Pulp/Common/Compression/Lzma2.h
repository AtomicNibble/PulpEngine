#pragma once


#ifndef X_COMPRESSION_LZMA2_H_
#define X_COMPRESSION_LZMA2_H_

X_NAMESPACE_BEGIN(core)

namespace Compression
{

	class LZMA
	{
	public:
		X_DECLARE_ENUM(CompressLevel)(
			LOW, // speed
			NORMAL, // normal
			HIGH // best
		);

	protected:
		//	virtual ~LZMA();

	public:
		// max source buffer size.
		static size_t maxSourceSize(void);
		// buffer than source is garanted to fit into.
		static size_t requiredDeflateDestBuf(size_t sourceLen);

		// none buffed single step inflate / deflate.
		static bool deflate(const void* pSrcBuf, size_t srcBufLen,
			void* pDstBuf, size_t destBufLen, size_t& destLenOut, CompressLevel::Enum lvl = CompressLevel::NORMAL);

		static bool inflate(void* pSrcBuf, size_t srcBufLen,
			void* pDstBuf, size_t destBufLen);


	private:
		X_NO_CREATE(LZMA);
		X_NO_COPY(LZMA);
		X_NO_ASSIGN(LZMA);
	};




} // namespace Compression

X_NAMESPACE_END

#endif // !X_COMPRESSION_LZMA2_H_