#include <EngineCommon.h>
#include "Zlib.h"

extern "C" {
#include <../../3rdparty/source/zlib-1.2.8/zlib.h>
}

// link zlib plz.
#if X_DEBUG
X_LINK_LIB("zlibstatd");
#else
X_LINK_LIB("zlibstat");
#endif // !X_DEBUG



X_NAMESPACE_BEGIN(core)

namespace Compression
{
	namespace
	{

		static void* StaticAlloc(void* opaque, uInt items, uInt size)
		{
			X_ASSERT_NOT_NULL(opaque);
			core::MemoryArenaBase* pArena = reinterpret_cast<core::MemoryArenaBase*>(opaque);

			return X_NEW_ARRAY(uint8_t, items* size, pArena, "ZlibInternalData");
		}

		static void StaticFree(void* opaque, void* address)
		{
			X_ASSERT_NOT_NULL(opaque);
			X_ASSERT_NOT_NULL(address);
			core::MemoryArenaBase* pArena = reinterpret_cast<core::MemoryArenaBase*>(opaque);

			X_DELETE(address, pArena);
		}


		int32_t CompLvlToZliblvl(Zlib::CompressLevel::Enum lvl)
		{
			switch (lvl)
			{
				case Zlib::CompressLevel::LOW:
					return Z_BEST_SPEED;
				case Zlib::CompressLevel::NORMAL:
					return Z_DEFAULT_COMPRESSION;
				case Zlib::CompressLevel::HIGH:
					return Z_BEST_COMPRESSION;
#if X_DEBUG
				default:
					X_ASSERT_UNREACHABLE();
					return Z_DEFAULT_COMPRESSION;
#else
					X_NO_SWITCH_DEFAULT();
#endif
			}
		}

		static const char* ZlibErrToStr(int32_t err)
		{
			switch (err)
			{
				case Z_OK:
					return "no Error";
				case Z_STREAM_END:
					return "stream ended";
				case Z_NEED_DICT:
					return "need dict";
				case Z_ERRNO:
					return "ERRNO";
				case Z_STREAM_ERROR:
					return "stream error";
				case Z_DATA_ERROR:
					return "data error";
				case Z_MEM_ERROR:
					return "mem error";
				case Z_BUF_ERROR:
					return "buffer error";
				case Z_VERSION_ERROR:
					return "version error";

				default:
					break;
			}

			return "Unknown Error";
		}


	} // namespace


	Zlib::Zlib()
	{
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pArena);
		stream_ = X_NEW(z_stream,gEnv->pArena,"ZlibStream");
		core::zero_this(stream_);
	}

	Zlib::~Zlib()
	{
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pArena);
		X_DELETE_AND_NULL(stream_, gEnv->pArena);
	}


	// -------------------------------------------------------------

	size_t Zlib::requiredDeflateDestBuf(size_t sourceLen)
	{
		return deflateBound(nullptr, safe_static_cast<uint32_t, size_t>(sourceLen));
	}

	bool Zlib::deflate(const void* pSrcBuf, size_t srcBufLen, 
		void* pDstBuf, size_t destBufLen, size_t& destLenOut, CompressLevel::Enum lvl)
	{
		// required
		X_ASSERT_NOT_NULL(pSrcBuf);
		X_ASSERT_NOT_NULL(pDstBuf);
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pArena);

		// early out.
		if (srcBufLen == 0) {
			X_WARNING("Zlib", "defalte called with empty src buffer");
			return false;
		}

		// check deflate size is valid.
		if (destBufLen < requiredDeflateDestBuf(srcBufLen)) {
			X_ERROR("Zlib", "Destination buffer is too small. require: %x recived: %x",
				requiredDeflateDestBuf(srcBufLen), destBufLen);
			return false;
		}

		// check destbuf len is less than 4gb.
		// the api i expose uses size_t so i don't have to cast stuff when using it
		// i just check here instead.
		// also no point checking src since it's equal or lest to dest.
		if (destBufLen > std::numeric_limits<uint32_t>::max()) {
			// lel how much ram do you have to get (2 * 4gb+) congiguious buffers.	
			X_ERROR("Zlib", "can't compress buffers greater than 4gb.");
			return false;
		}

		destLenOut = 0;

		z_stream stream;
		core::zero_object(stream);

		// cast away the cost since zlib type is not const.
		// even tho it don't edit the buffer.
		stream.next_in = reinterpret_cast<uint8_t*>(const_cast<void*>(pSrcBuf));
		stream.avail_in = safe_static_cast<uint32_t>(srcBufLen);

		stream.next_out = reinterpret_cast<uint8_t*>(pDstBuf);
		stream.avail_out = safe_static_cast<uint32_t>(destBufLen);

		stream.zalloc = StaticAlloc;
		stream.zfree = StaticFree;
		stream.opaque = gEnv->pArena;

		::deflateInit(&stream, CompLvlToZliblvl(lvl));

		int res = ::deflate(&stream, Z_FINISH);

		uint32_t deflatedSize = safe_static_cast<uint32_t>(destBufLen)-stream.avail_out;

		// always close.
		::deflateEnd(&stream);

		if (res != Z_STREAM_END)
		{
			X_ERROR("Zlib", "defalte error: %i -> %s", res, ZlibErrToStr(res));
			return false;
		}
		else
		{
			destLenOut = deflatedSize;
		}

		return true;
	}

	bool Zlib::inflate(void* pSrcBuf, size_t srcBufLen, 
		void* pDstBuf, size_t destBufLen)
	{
		// required
		X_ASSERT_NOT_NULL(pSrcBuf);
		X_ASSERT_NOT_NULL(pDstBuf);
		X_ASSERT_NOT_NULL(gEnv);
		X_ASSERT_NOT_NULL(gEnv->pArena);

		z_stream stream;
		core::zero_object(stream);

		// cast away the cost since zlib type is not const.
		// even tho it don't edit the buffer.
		stream.next_in = reinterpret_cast<uint8_t*>(const_cast<void*>(pSrcBuf));
		stream.avail_in = safe_static_cast<uint32_t>(srcBufLen);

		stream.next_out = reinterpret_cast<uint8_t*>(pDstBuf);
		stream.avail_out = safe_static_cast<uint32_t>(destBufLen);

		stream.zalloc = StaticAlloc;
		stream.zfree = StaticFree;
		stream.opaque = gEnv->pArena;

		::inflateInit(&stream);

		int res = ::inflate(&stream, Z_SYNC_FLUSH);

		::inflateEnd(&stream);

		if (res != Z_STREAM_END)
		{
			X_ERROR("Zlib", "inflate error: %i -> %s", res, ZlibErrToStr(res));
			return false;
		}

		return true;
	}

	// --------------------------------------

	ZlibInflate::ZlibInflate(void* pDst, size_t destLen) :
		pDst_(pDst), destLen_(destLen)
	{
		X_ASSERT_NOT_NULL(stream_);

		::inflateInit(stream_);

		stream_->next_out = reinterpret_cast<uint8_t*>(pDst);
		stream_->avail_out = safe_static_cast<uint32_t>(destLen);

		stream_->zalloc = StaticAlloc;
		stream_->zfree = StaticFree;
		stream_->opaque = gEnv->pArena;
	}

	ZlibInflate::~ZlibInflate()
	{
		::inflateEnd(stream_);
	}


	ZlibInflate::InflateResult::Enum ZlibInflate::Inflate(const void* pCompessedData, size_t len)
	{
		X_ASSERT_NOT_NULL(pCompessedData);

		if (len == 0)
			return InflateResult::OK;

		if (stream_->avail_out == 0)
			return InflateResult::DST_BUF_FULL;

		stream_->next_in = reinterpret_cast<uint8_t*>(const_cast<void*>(pCompessedData));
		stream_->avail_in = safe_static_cast<uint32_t>(len);

		// inflate it baby.
		int res = ::inflate(stream_, Z_BLOCK);

		uint32_t left = stream_->avail_out;

		if (res == Z_STREAM_END) {
			if (left > 0) {
				X_WARNING("Zlib", "buffer inflate reached stream end"
					", yet %s spare bytes left in dst buffer", left);
			}
			return InflateResult::DONE;
		}
		
		if (res != Z_OK) {
			X_ERROR("Zlib", "inflate error: %i -> %s", res, ZlibErrToStr(res));
			return InflateResult::ERROR;
		}

		return InflateResult::OK;
	}


} // namespace Compression

X_NAMESPACE_END