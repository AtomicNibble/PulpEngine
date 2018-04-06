#pragma once

#ifndef X_COMPRESSION_ZLIB_H_
#define X_COMPRESSION_ZLIB_H_

extern "C" {
struct z_stream_s;
};

#include <CompileTime\IsPOD.h>
#include <Containers\Array.h>
#include <ICompression.h>

#include <Util\Function.h>

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
        static Algo::Enum getAlgo(void);

        static size_t maxSourceSize(void);

        // none buffed single step inflate / deflate.
        static size_t requiredDeflateDestBuf(size_t sourceLen);

        static bool deflate(core::MemoryArenaBase* arena, const void* pSrcBuf, size_t srcBufLen,
            void* pDstBuf, size_t destBufLen, size_t& destLenOut, CompressLevel::Enum lvl = CompressLevel::NORMAL);

        static bool inflate(core::MemoryArenaBase* arena, const void* pSrcBuf, size_t srcBufLen,
            void* pDstBuf, size_t destBufLen);

        template<typename T>
        static bool deflate(core::MemoryArenaBase* arena, const core::Array<T>& data,
            core::Array<uint8_t>& compressed,
            CompressLevel::Enum lvl = CompressLevel::NORMAL);

        template<typename T>
        static bool inflate(core::MemoryArenaBase* arena, const core::Array<uint8_t>& data,
            core::Array<T>& inflated);

    private:
        X_NO_CREATE(Zlib);
        X_NO_COPY(Zlib);
        X_NO_ASSIGN(Zlib);
    };

    // --------------------------------------------------------

    template<typename T>
    X_INLINE bool Zlib::deflate(core::MemoryArenaBase* arena, const core::Array<T>& data, core::Array<uint8_t>& compressed,
        CompressLevel::Enum lvl)
    {
        static_assert(compileTime::IsPOD<T>::Value, "T must be a POD type.");

        size_t compressedSize = 0;
        size_t bufSize = requiredDeflateDestBuf(data.size() * sizeof(T));

        compressed.resize(bufSize);

        bool res = deflate(arena, data.ptr(), data.size() * sizeof(T),
            compressed.ptr(), compressed.size(), compressedSize, lvl);

        compressed.resize(compressedSize);
        return res;
    }

    template<typename T>
    X_INLINE bool Zlib::inflate(core::MemoryArenaBase* arena, const core::Array<uint8_t>& data, core::Array<T>& inflated)
    {
        static_assert(compileTime::IsPOD<T>::Value, "T must be a POD type.");

        return inflate(arena, data.ptr(), data.size(), inflated.ptr(), inflated.size() * sizeof(T));
    }

    // can take one or many inputs and inflate them into dest.
    class ZlibInflate
    {
        X_NO_COPY(ZlibInflate);
        X_NO_ASSIGN(ZlibInflate);

    public:
        X_DECLARE_ENUM(Result)
        (ERROR, OK, DONE);

        static const size_t DEFAULT_BUF_SIZE = 1024 * 16;

        typedef core::Function<void(const uint8_t* pData, size_t len, size_t inflatedOffset), 64> InflateCallback;

    public:
        ZlibInflate(core::MemoryArenaBase* arena, InflateCallback&& inflateCallback);
        ~ZlibInflate();

        void setBufferSize(size_t size);
        size_t inflatedSize(void) const;

        Result::Enum Inflate(const void* pCompessedData, size_t len);

    private:
        InflateCallback callback_;

    private:
        core::MemoryArenaBase* arena_;
        core::Array<uint8_t> buffer_;
        z_stream_s* stream_;
    };

    // can compress into blocks.
    class ZlibDefalte
    {
        X_NO_COPY(ZlibDefalte);
        X_NO_ASSIGN(ZlibDefalte);

    public:
        typedef ZlibInflate::Result Result;

        static const size_t DEFAULT_BUF_SIZE = 1024 * 16;

        typedef core::Function<void(const uint8_t* pData, size_t len, size_t deflatedOffset), 64> DeflateCallback;

    public:
        ZlibDefalte(core::MemoryArenaBase* arena, DeflateCallback&& defalteCallBack, CompressLevel::Enum lvl = CompressLevel::NORMAL);
        ~ZlibDefalte();

        void setBufferSize(size_t size);
        size_t deflatedSize(void) const;

        // this will accept N input blocks and call the deflate callback everytime the buffer is full or we are flusing (finish == true)
        // you can just pass a huge src block with finish == true and the callback will keep been called with blocks untill it's finished.
        Result::Enum Deflate(const void* pSrcData, size_t len, bool finish);

    private:
        DeflateCallback callback_;

    private:
        core::MemoryArenaBase* arena_;
        core::Array<uint8_t> buffer_;
        z_stream_s* stream_;
    };

} // namespace Compression

X_NAMESPACE_END

#endif // !X_COMPRESSION_ZLIB_H_