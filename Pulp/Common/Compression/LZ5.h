#pragma once

#ifndef X_COMPRESSION_LZ5_H_
#define X_COMPRESSION_LZ5_H_

#include <CompileTime\IsPOD.h>
#include <Containers\Array.h>

#include <ICompression.h>

X_NAMESPACE_BEGIN(core)

namespace Compression
{
    // this a struct not a namespace so it can be wrapped.
    class LZ5
    {
    public:
        static Algo::Enum getAlgo(void);

        // max source buffer size.
        static size_t maxSourceSize(void);
        // buffer than source is garanted to fit into.
        static size_t requiredDeflateDestBuf(size_t sourceLen);

        // none buffed single step inflate / deflate.
        static bool deflate(core::MemoryArenaBase* arena, const void* pSrcBuf, size_t srcBufLen,
            void* pDstBuf, size_t destBufLen, size_t& destLenOut,
            CompressLevel::Enum lvl = CompressLevel::NORMAL);

        static bool inflate(core::MemoryArenaBase* arena, const void* pSrcBuf, size_t srcBufLen,
            void* pDstBuf, size_t destBufLen);

        template<typename T>
        static bool deflate(core::MemoryArenaBase* arena, const core::Array<T>& data,
            core::Array<uint8_t>& compressed, CompressLevel::Enum lvl = CompressLevel::NORMAL);

        template<typename T>
        static bool inflate(core::MemoryArenaBase* arena, const core::Array<uint8_t>& data,
            core::Array<T>& inflated);

    private:
        X_NO_CREATE(LZ5);
        X_NO_COPY(LZ5);
        X_NO_ASSIGN(LZ5);
    };

    class LZ5HC : private LZ5
    {
    public:
        static Algo::Enum getAlgo(void);

        using LZ5::maxSourceSize;
        using LZ5::requiredDeflateDestBuf;
        using LZ5::inflate;

        // none buffed single step inflate / deflate.
        static bool deflate(core::MemoryArenaBase* arena, const void* pSrcBuf, size_t srcBufLen,
            void* pDstBuf, size_t destBufLen, size_t& destLenOut,
            CompressLevel::Enum lvl = CompressLevel::NORMAL);

        template<typename T>
        static bool deflate(core::MemoryArenaBase* arena, const core::Array<T>& data,
            core::Array<uint8_t>& compressed, CompressLevel::Enum lvl = CompressLevel::NORMAL);

    private:
        X_NO_CREATE(LZ5HC);
        X_NO_COPY(LZ5HC);
        X_NO_ASSIGN(LZ5HC);
    };

    /*
	This is for streaming raw data to compressed blocks.
	this allows for compressing large blocks with low overhead.

	*/
    class LZ5Stream
    {
    public:
        LZ5Stream(core::MemoryArenaBase* arena, CompressLevel::Enum lvl);
        ~LZ5Stream();

        // this are validated by static asset in source file.
        X_INLINE static constexpr size_t maxSourceSize(void)
        {
            return 0x7E000000;
        }
        X_INLINE static constexpr size_t requiredDeflateDestBuf(size_t size)
        {
            return (size > maxSourceSize() ? 0 : (size) + 1 + 1 + ((size / (1 << 17)) + 1) * 4);
        }

        bool loadDict(const uint8_t* pDict, size_t size);

        size_t compressContinue(const void* pSrcBuf, size_t srcBufLen, void* pDstBuf, size_t destBufLen);

    private:
        void* stream_;
    };

    class LZ5StreamDecode
    {
    public:
        LZ5StreamDecode();
        ~LZ5StreamDecode();

        bool loadDict(const uint8_t* pDict, size_t size);

        size_t decompressContinue(const void* pSrcBuf, void* pDstBuf, size_t compressedSize, size_t maxDecompressedSize);

    private:
        size_t decodeStream_[4];
    };

    // --------------------------------------------------------

    template<typename T>
    X_INLINE bool LZ5::deflate(core::MemoryArenaBase* arena, const core::Array<T>& data,
        core::Array<uint8_t>& compressed, CompressLevel::Enum lvl)
    {
        static_assert(compileTime::IsPOD<T>::Value, "T must be a POD type.");

        size_t compressedSize = 0;
        size_t bufSize = requiredDeflateDestBuf((data.size() * sizeof(T)));

        compressed.resize(bufSize);

        bool res = deflate(arena, data.ptr(), data.size() * sizeof(T),
            compressed.ptr(), compressed.size(), compressedSize, lvl);

        compressed.resize(compressedSize);
        return res;
    }

    template<typename T>
    X_INLINE bool LZ5::inflate(core::MemoryArenaBase* arena, const core::Array<uint8_t>& data, core::Array<T>& inflated)
    {
        static_assert(compileTime::IsPOD<T>::Value, "T must be a POD type.");

        return inflate(arena, data.ptr(), data.size(), inflated.ptr(), inflated.size() * sizeof(T));
    }

    // --------------------------------------------------------

    template<typename T>
    X_INLINE bool LZ5HC::deflate(core::MemoryArenaBase* arena, const core::Array<T>& data,
        core::Array<uint8_t>& compressed, CompressLevel::Enum lvl)
    {
        static_assert(compileTime::IsPOD<T>::Value, "T must be a POD type.");

        size_t compressedSize = 0;
        size_t bufSize = requiredDeflateDestBuf((data.size() * sizeof(T)));

        compressed.resize(bufSize);

        bool res = deflate(arena, data.ptr(), data.size() * sizeof(T),
            compressed.ptr(), compressed.size(), compressedSize, lvl);

        compressed.resize(compressedSize);
        return res;
    }

} // namespace Compression

X_NAMESPACE_END

#endif // !X_COMPRESSION_LZ5_H_