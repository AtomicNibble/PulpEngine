#include <EngineCommon.h>
#include "LZ4.h"

#include <../../3rdparty/source/lz4-1.7.4.2/lz4_lib.h>
#include <../../3rdparty/source/lz4-1.7.4.2/lz4hc.h>

X_NAMESPACE_BEGIN(core)

namespace Compression
{
    namespace
    {
        int compressLevelToAcceleration(CompressLevel::Enum lvl)
        {
            if (lvl == CompressLevel::LOW) {
                return 8;
            }
            if (lvl == CompressLevel::NORMAL) {
                return 3;
            }
            if (lvl == CompressLevel::HIGH) {
                return 1;
            }

            X_ASSERT_UNREACHABLE();
            return 1;
        }

        int compressLevelToAccelerationHC(CompressLevel::Enum lvl)
        {
            if (lvl == CompressLevel::LOW) {
                return 3;
            }
            if (lvl == CompressLevel::NORMAL) {
                return 9;
            }
            if (lvl == CompressLevel::HIGH) {
                return 12;
            }

            X_ASSERT_UNREACHABLE();
            return 1;
        }

    } // namespace

    Algo::Enum LZ4::getAlgo(void)
    {
        return Algo::LZ4;
    }

    size_t LZ4::maxSourceSize(void)
    {
        return static_cast<size_t>(LZ4_MAX_INPUT_SIZE);
    }

    size_t LZ4::requiredDeflateDestBuf(size_t sourceLen)
    {
        size_t destLen = static_cast<size_t>(LZ4_compressBound(safe_static_cast<int32_t>(sourceLen)));
        X_ASSERT(destLen >= sourceLen, "Potential wrap around, dest is smaller than source")(sourceLen, destLen);
        return destLen;
    }

    bool LZ4::deflate(const void* pSrcBuf, size_t srcBufLen,
        void* pDstBuf, size_t destBufLen, size_t& destLenOut, CompressLevel::Enum lvl)
    {
        const char* pSrc = reinterpret_cast<const char*>(pSrcBuf);
        char* pDst = reinterpret_cast<char*>(pDstBuf);

        const int32_t srcSize = safe_static_cast<int, size_t>(srcBufLen);
        const int32_t detSize = safe_static_cast<int, size_t>(destBufLen);

        const int32_t res = LZ4_compress_fast(pSrc, pDst, srcSize, detSize,
            compressLevelToAcceleration(lvl));

        if (res <= 0) {
            X_ERROR("LZ4", "Failed to compress buffer: %" PRIi32, res);
            destLenOut = 0;
            return false;
        }

        destLenOut = res;
        return true;
    }

    bool LZ4::inflate(const void* pSrcBuf, size_t srcBufLen,
        void* pDstBuf, size_t destBufLen)
    {
        X_UNUSED(srcBufLen);

        const char* pSrc = reinterpret_cast<const char*>(pSrcBuf);
        char* pDst = reinterpret_cast<char*>(pDstBuf);
        const int32_t size = safe_static_cast<int, size_t>(destBufLen);

        const int32_t res = LZ4_decompress_fast(pSrc, pDst, size);

        if (res <= 0) {
            X_ERROR("LZ4", "Failed to decompress buffer: %" PRIi32, res);
            return false;
        }
        return true;
    }

    // ---------------------------------------

    bool LZ4HC::deflate(const void* pSrcBuf, size_t srcBufLen,
        void* pDstBuf, size_t destBufLen, size_t& destLenOut, CompressLevel::Enum lvl)
    {
        const char* pSrc = reinterpret_cast<const char*>(pSrcBuf);
        char* pDst = reinterpret_cast<char*>(pDstBuf);

        const int32_t srcSize = safe_static_cast<int, size_t>(srcBufLen);
        const int32_t detSize = safe_static_cast<int, size_t>(destBufLen);

        const int32_t res = LZ4_compress_HC(pSrc, pDst, srcSize, detSize, compressLevelToAccelerationHC(lvl));

        if (res <= 0) {
            X_ERROR("LZ4", "Failed to compress buffer: %" PRIi32, res);
            destLenOut = 0;
            return false;
        }

        destLenOut = res;
        return true;
    }

    // ---------------------------------------

    // check the helpers are correct.

    static_assert(LZ4Stream::maxSourceSize() == LZ4_MAX_INPUT_SIZE, "LZ4 max source size helper don't match lib value");
    static_assert(LZ4Stream::requiredDeflateDestBuf(0x1223345) == LZ4_COMPRESSBOUND(0x1223345), "LZ4 compressbound helper don't match lib result");

    LZ4Stream::LZ4Stream(core::MemoryArenaBase* arena) :
        arena_(arena),
        stream_(nullptr)
    {
        LZ4_stream_t* pStream = X_NEW(LZ4_stream_t, arena, "LZ4Stream");
        LZ4_resetStream(pStream);

        stream_ = pStream;
    }

    LZ4Stream::~LZ4Stream()
    {
        if (stream_) {
            X_DELETE(reinterpret_cast<LZ4_stream_t*>(stream_), arena_);
        }
    }

    bool LZ4Stream::loadDict(const uint8_t* pDict, size_t size)
    {
        if (size < sizeof(SharedDictHdr)) {
            X_ERROR("LZ4", "Dictionary is too small to be valid");
            return false;
        }

        const SharedDictHdr* pDictHdr = reinterpret_cast<const SharedDictHdr*>(pDict);
        if (!pDictHdr->IsMagicValid()) {
            X_ERROR("LZ4", "Dictionary header is not valid");
            return false;
        }

        const int32_t cappedSize = core::Min(safe_static_cast<int32_t>(size), 64 * 1024);

        int32_t dictSize = LZ4_loadDict(
            reinterpret_cast<LZ4_stream_t*>(stream_),
            reinterpret_cast<const char*>(pDict),
            cappedSize);

        if (dictSize == 0 || dictSize < cappedSize) {
            X_ERROR("LZ4", "Failed to set dictionary");
            return false;
        }

        return true;
    }

    size_t LZ4Stream::compressContinue(const void* pSrcBuf, size_t srcBufLen, void* pDstBuf, size_t destBufLen,
        CompressLevel::Enum lvl)
    {
        X_ASSERT_NOT_NULL(stream_);
        LZ4_stream_t* pStream = reinterpret_cast<LZ4_stream_t*>(stream_);

        const char* pSrc = reinterpret_cast<const char*>(pSrcBuf);
        char* pDst = reinterpret_cast<char*>(pDstBuf);
        int srcSize = safe_static_cast<int, size_t>(srcBufLen);
        int dstSize = safe_static_cast<int, size_t>(destBufLen);

        int res = LZ4_compress_fast_continue(pStream, pSrc, pDst, srcSize, dstSize,
            compressLevelToAcceleration(lvl));

        if (res == 0) {
            X_ERROR("LZ4", "Failed to compress buffer");
        }

        return res;
    }

    // ---------------------------------------

    LZ4StreamDecode::LZ4StreamDecode()
    {
        static_assert(sizeof(decodeStream_) == sizeof(LZ4_streamDecode_t), "");
        LZ4_streamDecode_t* pStream = reinterpret_cast<LZ4_streamDecode_t*>(decodeStream_);

        LZ4_setStreamDecode(pStream, nullptr, 0);
    }

    LZ4StreamDecode::~LZ4StreamDecode()
    {
#if X_DEBUG
        core::zero_object(decodeStream_);
#endif // !X_DEBUG
    }

    bool LZ4StreamDecode::loadDict(const uint8_t* pDict, size_t size)
    {
        if (size < sizeof(SharedDictHdr)) {
            X_ERROR("LZ4", "Dictionary is too small to be valid");
            return false;
        }

        const SharedDictHdr* pDictHdr = reinterpret_cast<const SharedDictHdr*>(pDict);
        if (!pDictHdr->IsMagicValid()) {
            X_ERROR("LZ4", "Dictionary header is not valid");
            return false;
        }

        const int32_t cappedSize = core::Min(safe_static_cast<int32_t>(size), 64 * 1024);

        int32_t res = LZ4_setStreamDecode(
            reinterpret_cast<LZ4_streamDecode_t*>(decodeStream_),
            reinterpret_cast<const char*>(pDict),
            cappedSize);

        return res == 1;
    }

    size_t LZ4StreamDecode::decompressContinue(const void* pSrcBuf, void* pDstBuf, size_t originalSize)
    {
        LZ4_streamDecode_t* pStream = reinterpret_cast<LZ4_streamDecode_t*>(decodeStream_);

        const char* pSrc = reinterpret_cast<const char*>(pSrcBuf);
        char* pDst = reinterpret_cast<char*>(pDstBuf);
        int origSize = safe_static_cast<int, size_t>(originalSize);

        int res = LZ4_decompress_fast_continue(pStream, pSrc, pDst, origSize);

        if (res != origSize) {
            X_ERROR("LZ4", "Failed to decompress buffer");
        }

        return res;
    }

    size_t LZ4StreamDecode::decompressContinue(const void* pSrcBuf, void* pDstBuf, size_t compressedSize, size_t maxDecompressedSize)
    {
        LZ4_streamDecode_t* pStream = reinterpret_cast<LZ4_streamDecode_t*>(decodeStream_);

        const char* pSrc = reinterpret_cast<const char*>(pSrcBuf);
        char* pDst = reinterpret_cast<char*>(pDstBuf);
        int cmpSize = safe_static_cast<int, size_t>(compressedSize);
        int maxDecSize = safe_static_cast<int, size_t>(maxDecompressedSize);

        int res = LZ4_decompress_safe_continue(pStream, pSrc, pDst, cmpSize, maxDecSize);

        return res;
    }

} // namespace Compression

X_NAMESPACE_END