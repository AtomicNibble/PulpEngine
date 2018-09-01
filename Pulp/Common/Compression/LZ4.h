#pragma once

#ifndef X_COMPRESSION_LZ4_H_
#define X_COMPRESSION_LZ4_H_

#include <CompileTime\IsPOD.h>
#include <Containers\Array.h>

#include <ICompression.h>

X_NAMESPACE_BEGIN(core)

namespace Compression
{
    // this a struct not a namespace so it can be wrapped.
    class LZ4
    {
    public:
        static Algo::Enum getAlgo(void);

        // max source buffer size.
        static size_t maxSourceSize(void);
        // buffer than source is garanted to fit into.
        static size_t requiredDeflateDestBuf(size_t sourceLen);

        // none buffed single step inflate / deflate.
        static bool deflate(const void* pSrcBuf, size_t srcBufLen, void* pDstBuf, size_t destBufLen, size_t& destLenOut,
            CompressLevel::Enum lvl = CompressLevel::NORMAL);

        static bool inflate(const void* pSrcBuf, size_t srcBufLen, void* pDstBuf, size_t destBufLen);

        template<typename T>
        X_INLINE static bool deflate(const core::Array<T>& data,
            core::Array<uint8_t>& compressed, CompressLevel::Enum lvl = CompressLevel::NORMAL);

        template<typename T>
        X_INLINE static bool inflate(const core::Array<uint8_t>& data,
            core::Array<T>& inflated);

        // overlords taking swap arena (unused).
        X_INLINE static bool deflate(core::MemoryArenaBase* arena, const void* pSrcBuf, size_t srcBufLen,
            void* pDstBuf, size_t destBufLen, size_t& destLenOut,
            CompressLevel::Enum lvl = CompressLevel::NORMAL);

        X_INLINE static bool inflate(core::MemoryArenaBase* arena, const void* pSrcBuf, size_t srcBufLen,
            void* pDstBuf, size_t destBufLen);

        template<typename T>
        X_INLINE static bool deflate(core::MemoryArenaBase* arena, const core::Array<T>& data,
            core::Array<uint8_t>& compressed, CompressLevel::Enum lvl = CompressLevel::NORMAL);

        template<typename T>
        X_INLINE static bool inflate(core::MemoryArenaBase* arena, const core::Array<uint8_t>& data,
            core::Array<T>& inflated);

    private:
        X_NO_CREATE(LZ4);
        X_NO_COPY(LZ4);
        X_NO_ASSIGN(LZ4);
    };

    class LZ4HC : public LZ4
    {
    public:
        // none buffed single step inflate / deflate.
        static bool deflate(const void* pSrcBuf, size_t srcBufLen,
            void* pDstBuf, size_t destBufLen, size_t& destLenOut,
            CompressLevel::Enum lvl = CompressLevel::NORMAL);

        template<typename T>
        X_INLINE static bool deflate(const core::Array<T>& data,
            core::Array<uint8_t>& compressed, CompressLevel::Enum lvl = CompressLevel::NORMAL);

        // overlords taking swap arena (unused).
        X_INLINE static bool deflate(core::MemoryArenaBase* arena, const void* pSrcBuf, size_t srcBufLen,
            void* pDstBuf, size_t destBufLen, size_t& destLenOut,
            CompressLevel::Enum lvl = CompressLevel::NORMAL);

        template<typename T>
        X_INLINE static bool deflate(core::MemoryArenaBase* arena, const core::Array<T>& data,
            core::Array<uint8_t>& compressed, CompressLevel::Enum lvl = CompressLevel::NORMAL);

    private:
        X_NO_CREATE(LZ4HC);
        X_NO_COPY(LZ4HC);
        X_NO_ASSIGN(LZ4HC);
    };

    /*
		This is for streaming raw data to compressed blocks.
		this allows for compressing large blocks with low overhead.

	*/
    class LZ4Stream
    {
    public:
        LZ4Stream(core::MemoryArenaBase* arena);
        ~LZ4Stream();

        // this are validated by static asset in source file.
        X_INLINE static constexpr size_t maxSourceSize(void)
        {
            return 0x7E000000;
        }
        X_INLINE static constexpr size_t requiredDeflateDestBuf(size_t size)
        {
            return (size > maxSourceSize() ? 0 : (size) + ((size) / 255) + 16);
        }

        bool loadDict(const uint8_t* pDict, size_t size);

        size_t compressContinue(const void* pSrcBuf, size_t srcBufLen, void* pDstBuf, size_t destBufLen,
            CompressLevel::Enum lvl = CompressLevel::NORMAL);

    private:
        core::MemoryArenaBase* arena_;
        void* stream_;
    };

    class LZ4StreamDecode
    {
    public:
        LZ4StreamDecode();
        ~LZ4StreamDecode();

        bool loadDict(const uint8_t* pDict, size_t size);

        // originalSize is the size of the decompressed data.
        size_t decompressContinue(const void* pSrcBuf, void* pDstBuf, size_t originalSize);
        size_t decompressContinue(const void* pSrcBuf, void* pDstBuf, size_t compressedSize, size_t maxDecompressedSize);

    private:
        uint64_t decodeStream_[4];
    };

    // --------------------------------------------------------

    template<typename T>
    X_INLINE bool LZ4::deflate(const core::Array<T>& data,
        core::Array<uint8_t>& compressed, CompressLevel::Enum lvl)
    {
        static_assert(compileTime::IsPOD<T>::Value, "T must be a POD type.");

        size_t compressedSize = 0;
        size_t bufSize = requiredDeflateDestBuf((data.size() * sizeof(T)));

        compressed.resize(bufSize);

        bool res = deflate(data.ptr(), data.size() * sizeof(T),
            compressed.ptr(), compressed.size(), compressedSize, lvl);

        compressed.resize(compressedSize);
        return res;
    }

    template<typename T>
    X_INLINE bool LZ4::inflate(const core::Array<uint8_t>& data, core::Array<T>& inflated)
    {
        static_assert(compileTime::IsPOD<T>::Value, "T must be a POD type.");

        return inflate(data.ptr(), data.size(), inflated.ptr(), inflated.size() * sizeof(T));
    }

    // strip swap area
    X_INLINE bool LZ4::deflate(core::MemoryArenaBase* arena, const void* pSrcBuf, size_t srcBufLen,
        void* pDstBuf, size_t destBufLen, size_t& destLenOut, CompressLevel::Enum lvl)
    {
        X_UNUSED(arena);

        return deflate(pSrcBuf, srcBufLen, pDstBuf, destBufLen, destLenOut, lvl);
    }

    X_INLINE bool LZ4::inflate(core::MemoryArenaBase* arena, const void* pSrcBuf, size_t srcBufLen,
        void* pDstBuf, size_t destBufLen)
    {
        X_UNUSED(arena);

        return inflate(pSrcBuf, srcBufLen, pDstBuf, destBufLen);
    }

    template<typename T>
    X_INLINE bool LZ4::deflate(core::MemoryArenaBase* arena, const core::Array<T>& data,
        core::Array<uint8_t>& compressed, CompressLevel::Enum lvl)
    {
        X_UNUSED(arena);

        return deflate<T>(data, compressed, lvl);
    }

    template<typename T>
    X_INLINE bool LZ4::inflate(core::MemoryArenaBase* arena, const core::Array<uint8_t>& data, core::Array<T>& inflated)
    {
        X_UNUSED(arena);

        return inflate<T>(data, inflated);
    }


    // --------------------------------------------------------

    template<typename T>
    X_INLINE bool LZ4HC::deflate(const core::Array<T>& data,
        core::Array<uint8_t>& compressed, CompressLevel::Enum lvl)
    {
        static_assert(compileTime::IsPOD<T>::Value, "T must be a POD type.");

        size_t compressedSize = 0;
        size_t bufSize = requiredDeflateDestBuf((data.size() * sizeof(T)));

        compressed.resize(bufSize);

        bool res = deflate(data.ptr(), data.size() * sizeof(T),
            compressed.ptr(), compressed.size(), compressedSize, lvl);

        compressed.resize(compressedSize);
        return res;
    }

    X_INLINE bool LZ4HC::deflate(core::MemoryArenaBase* arena, const void* pSrcBuf, size_t srcBufLen,
        void* pDstBuf, size_t destBufLen, size_t& destLenOut, CompressLevel::Enum lvl)
    {
        X_UNUSED(arena);

        return deflate(pSrcBuf, srcBufLen, pDstBuf, destBufLen, destLenOut, lvl);
    }

    template<typename T>
    X_INLINE bool LZ4HC::deflate(core::MemoryArenaBase* arena, const core::Array<T>& data,
        core::Array<uint8_t>& compressed, CompressLevel::Enum lvl)
    {
        X_UNUSED(arena);

        return deflate<T>(data, compressed, lvl);
    }


} // namespace Compression

X_NAMESPACE_END

#endif // !X_COMPRESSION_LZ4_H_