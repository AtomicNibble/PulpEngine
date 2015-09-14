#pragma once


#ifndef X_COMPRESSION_LZ4_H_
#define X_COMPRESSION_LZ4_H_

X_NAMESPACE_BEGIN(core)

namespace Compression
{


	class LZ4
	{
	public:
		X_DECLARE_ENUM(CompressLevel)(
			LOW, // speed
			NORMAL, // normal
			HIGH // best
		);


	protected:
	//	virtual ~LZ4();

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
		X_NO_CREATE(LZ4);
		X_NO_COPY(LZ4);
		X_NO_ASSIGN(LZ4);
	};



} // namespace Compression

X_NAMESPACE_END

#endif // !X_COMPRESSION_LZ4_H_