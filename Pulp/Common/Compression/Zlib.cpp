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


		int32_t CompLvlToZliblvl(CompressLevel::Enum lvl)
		{
			switch (lvl)
			{
			case CompressLevel::LOW:
				return Z_BEST_SPEED;
			case CompressLevel::NORMAL:
				return Z_DEFAULT_COMPRESSION;
			case CompressLevel::HIGH:
				return Z_BEST_COMPRESSION;
#if X_DEBUG
			default:
				X_ASSERT_UNREACHABLE();
				return Z_DEFAULT_COMPRESSION;
			}
#else
					X_NO_SWITCH_DEFAULT;
			}
			return 0;
#endif
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


	// -------------------------------------------------------------
	Algo::Enum Zlib::getAlgo(void)
	{
		return Algo::ZLIB;
	}

	size_t Zlib::maxSourceSize(void)
	{
		// could not find real vlaue, so this may be wrong
		return std::numeric_limits<int32_t>::max();
	}

	size_t Zlib::requiredDeflateDestBuf(size_t sourceLen)
	{
		return deflateBound(nullptr, safe_static_cast<uint32_t, size_t>(sourceLen));
	}

	bool Zlib::deflate(core::MemoryArenaBase* arena, const void* pSrcBuf, size_t srcBufLen,
		void* pDstBuf, size_t destBufLen, size_t& destLenOut, CompressLevel::Enum lvl)
	{
		// required
		X_ASSERT_NOT_NULL(pSrcBuf);
		X_ASSERT_NOT_NULL(pDstBuf);

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
		stream.opaque = arena;

		::deflateInit(&stream, CompLvlToZliblvl(lvl));

		const int32_t res = ::deflate(&stream, Z_FINISH);

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

	bool Zlib::inflate(core::MemoryArenaBase* arena, const void* pSrcBuf, size_t srcBufLen,
		void* pDstBuf, size_t destBufLen)
	{
		// required
		X_ASSERT_NOT_NULL(pSrcBuf);
		X_ASSERT_NOT_NULL(pDstBuf);

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
		stream.opaque = arena;

		::inflateInit(&stream);

		const int32_t res = ::inflate(&stream, Z_SYNC_FLUSH);

		::inflateEnd(&stream);

		// If res == Z_OK that means we ran out of dest buffer beffore infalte all of src buffer.
		if (res != Z_STREAM_END)
		{
			X_ERROR("Zlib", "inflate error: %i -> %s", res, ZlibErrToStr(res));
			if (res == Z_OK) {
				X_ERROR("Zlib", "dest buffer is too small %i bytes left in src.", stream.avail_in);
			}
			return false;
		}

		return true;
	}


	// --------------------------------------

	ZlibInflate::ZlibInflate(core::MemoryArenaBase* arena, void* pDst, size_t destLen) :
		arena_(arena),
		stream_(nullptr),
		pDst_(pDst), 
		destLen_(destLen)
	{
		stream_ = X_NEW(z_stream, arena_, "ZlibStream");
		core::zero_this(stream_);

		X_ASSERT_NOT_NULL(stream_);

		stream_->zalloc = StaticAlloc;
		stream_->zfree = StaticFree;
		stream_->opaque = arena_;
	
		::inflateInit(stream_);

		stream_->next_out = reinterpret_cast<uint8_t*>(pDst);
		stream_->avail_out = safe_static_cast<uint32_t>(destLen);

	}

	ZlibInflate::~ZlibInflate()
	{
		::inflateEnd(stream_);
		X_DELETE_AND_NULL(stream_, arena_);
	}


	ZlibInflate::InflateResult::Enum ZlibInflate::Inflate(const void* pCompessedData, size_t len)
	{
		X_ASSERT_NOT_NULL(pCompessedData);

		if (len == 0)
			return InflateResult::OK;

		if (stream_->avail_out == 0) {
			X_ERROR("Zlib", "can't infalte any more dest buffer is full");
			return InflateResult::ERROR;
		}

		stream_->next_in = reinterpret_cast<uint8_t*>(const_cast<void*>(pCompessedData));
		stream_->avail_in = safe_static_cast<uint32_t>(len);

		// inflate it baby.
		const int32_t res = ::inflate(stream_, Z_SYNC_FLUSH);

		uint32_t left = stream_->avail_out;

		if (res == Z_STREAM_END || (left == 0 && res == Z_OK)) {
			if (left > 0) {
				X_WARNING("Zlib", "buffer inflate reached stream end"
					", yet %s spare bytes left in dst buffer", left);
			}
			return InflateResult::DONE;
		}
		
		if (res != Z_OK) {
			X_ERROR("Zlib", "inflate error: %" PRIi32 " -> %s", res, ZlibErrToStr(res));
			return InflateResult::ERROR;
		}

		return InflateResult::OK;
	}


	// --------------------------------------


	ZlibDefalte::ZlibDefalte(core::MemoryArenaBase* arena, DeflateCallback defalteCallBack, CompressLevel::Enum lvl) :
		callback_(defalteCallBack),
		arena_(arena),
		buffer_(arena)
	{
		stream_ = X_NEW(z_stream, arena_, "ZlibDelateStream");
		core::zero_this(stream_);

		X_ASSERT_NOT_NULL(stream_);

		stream_->zalloc = StaticAlloc;
		stream_->zfree = StaticFree;
		stream_->opaque = arena_;

		::deflateInit(stream_, CompLvlToZliblvl(lvl));
	}

	ZlibDefalte::~ZlibDefalte()
	{
		::deflateEnd(stream_);
		X_DELETE_AND_NULL(stream_, arena_);
	}

	void ZlibDefalte::setBufferSize(size_t size)
	{
		if (size < 1) {
			X_ERROR("Zlib", "Zero buffer size: %" PRIuS " reverting to default", size);
			size = DEFAULT_BUF_SIZE;
		}

		buffer_.resize(size);
	}

	ZlibDefalte::DeflateResult::Enum ZlibDefalte::Deflate(const void* pSrcData, size_t len, bool finish)
	{
		if (buffer_.isEmpty()) {
			buffer_.resize(DEFAULT_BUF_SIZE);
		}

		stream_->next_in = reinterpret_cast<uint8_t*>(const_cast<void*>(pSrcData));
		stream_->avail_in = safe_static_cast<uint32_t>(len);
		stream_->next_out = buffer_.data();
		stream_->avail_out = safe_static_cast<uint32_t>(buffer_.size());

		for(;;)
		{
			const int32_t res = ::deflate(stream_, finish ? Z_FINISH : Z_NO_FLUSH);

			// do we have a finished block?
			if (stream_->avail_out == 0)
			{
				callback_(buffer_.data(), buffer_.size());

				stream_->next_out = buffer_.data();
				stream_->avail_out = safe_static_cast<uint32_t>(buffer_.size());
			}
			
			if (res == Z_OK)
			{
				if (stream_->avail_in == 0)
				{
					return DeflateResult::OK;
				}

				// continue.
			}
			else if (res == Z_STREAM_END && finish)
			{
				const size_t bytesInBuf = (buffer_.size() - stream_->avail_out);

				callback_(buffer_.data(), bytesInBuf);

				stream_->next_out = nullptr;
				stream_->avail_out = 0;
				return DeflateResult::OK;
			}
			else
			{
				stream_->next_out = nullptr;
				stream_->avail_out = 0;
				X_ERROR("Zlib", "Deflate error % -> %s" PRIi32, res, ZlibErrToStr(res));
				return DeflateResult::ERROR;
			}
		}
	}


} // namespace Compression

X_NAMESPACE_END