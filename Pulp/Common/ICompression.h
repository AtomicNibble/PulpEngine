#pragma once

#ifndef _X_COMPRESSION_I_H_
#define _X_COMPRESSION_I_H_

#include <Containers\Array.h>
#include <Util\Span.h>

X_NAMESPACE_BEGIN(core)

namespace Compression
{
    X_DECLARE_ENUM8(Algo)(
        STORE, 
        LZ4, 
        LZ4HC, 
        LZMA, 
        ZLIB, 
        LZ5, 
        LZ5HC);

    X_DECLARE_ENUM(CompressLevel)
    (
        LOW,    // speed
        NORMAL, // normal
        HIGH    // best
    );

    X_DECLARE_FLAGS8(CompressFlag)
    (
        SHARED_DICT);

    typedef Flags8<CompressFlag> CompressFlags;
    typedef uint16_t SharedDictId;

    // placed at start of defalted buffers.
    // that are made via Compressor<T>
    struct BufferHdr
    {
        static const uint32_t MAGIC = 0x37BD98;

        BufferHdr()
        {
            core::zero_this(this);
        }

        X_INLINE bool IsMagicValid(void) const
        {
            uint32_t val = (magic[0] << 16 | magic[1] << 8 | magic[2]);
            return val == MAGIC;
        }

        Algo::Enum algo;
        uint8_t magic[3];

        CompressFlags flags;
        uint8_t _pad;
        SharedDictId sharedDictId;

        // we don't allow huge single deflated blocks.
        // i see no use case.
        uint32_t deflatedSize;
        uint32_t inflatedSize;
    };

    X_ENSURE_SIZE(BufferHdr, 16);

    struct SharedDictHdr
    {
        static const uint32_t MAGIC = 0x29C820F7;

        SharedDictHdr()
        {
            core::zero_this(this);
        }

        X_INLINE bool IsMagicValid(void) const
        {
            return magic == MAGIC;
        }

        uint32_t magic;
        uint32_t size;
        SharedDictId sharedDictId;
        uint16_t _pad[3];
    };

    X_ENSURE_SIZE(SharedDictHdr, 16);

    struct ICompressor
    {
        virtual ~ICompressor() = default;

        static Algo::Enum getAlgo(core::span<const uint8_t> data)
        {
            X_ASSERT(data.length() >= sizeof(BufferHdr), "Buffer is too small to contain BufferHdr")(sizeof(BufferHdr), data.length());
            const BufferHdr* pHdr = union_cast<const BufferHdr*, const uint8_t*>(data.data());
            X_ASSERT(pHdr->IsMagicValid(), "Compressed buffer header is not valid")();
            return pHdr->algo;
        }

        static bool validBuffer(core::span<const uint8_t> data)
        {
            if (data.length() < sizeof(BufferHdr)) {
                return false;
            }
            const BufferHdr* pHdr = union_cast<const BufferHdr*, const uint8_t*>(data.data());
            if (!pHdr->IsMagicValid()) {
                return false;
            }
            if (static_cast<uint32_t>(pHdr->algo) >= Algo::ENUM_COUNT) {
                return false;
            }

            return true;
        }

        static const BufferHdr* getBufferHdr(core::span<const uint8_t> data)
        {
            if (data.length() < sizeof(BufferHdr)) {
                return nullptr;
            }
            const BufferHdr* pHdr = union_cast<const BufferHdr*, const uint8_t*>(data.data());
            if (!pHdr->IsMagicValid()) {
                return nullptr;
            }
            if (static_cast<uint32_t>(pHdr->algo) >= Algo::ENUM_COUNT) {
                return nullptr;
            }

            return pHdr;
        }

        // max source buffer size.
        virtual size_t maxSourceSize(void) const X_ABSTRACT;
        // buffer than source is garanted to fit into.
        virtual size_t requiredDeflateDestBuf(size_t sourceLen) const X_ABSTRACT;

        template<typename T>
        bool deflate(core::MemoryArenaBase* arena, const core::Array<T>& data,
            core::Array<uint8_t>& compressed, CompressLevel::Enum lvl = CompressLevel::NORMAL);

        template<typename T>
        bool deflate(core::MemoryArenaBase* arena, core::span<const T> data,
            core::Array<uint8_t>& compressed, CompressLevel::Enum lvl = CompressLevel::NORMAL);

        template<typename T>
        bool inflate(core::MemoryArenaBase* arena, const core::Array<uint8_t>& data,
            core::Array<T>& inflated);

        template<typename T>
        bool inflate(core::MemoryArenaBase* arena, core::span<const uint8_t> data, core::Array<T>& inflated);

        bool inflate(core::MemoryArenaBase* arena, const uint8_t* pBegin, const uint8_t* pEnd,
            uint8_t* pInflatedBegin, uint8_t* pInflatedEnd);

    private:
        virtual bool deflate_int(core::MemoryArenaBase* arena, const void* pSrcBuf, size_t srcBufLen,
            void* pDstBuf, size_t destBufLen, size_t& destLenOut,
            CompressLevel::Enum lvl = CompressLevel::NORMAL) X_ABSTRACT;

        virtual bool inflate_int(core::MemoryArenaBase* arena, const void* pSrcBuf, size_t srcBufLen,
            void* pDstBuf, size_t destBufLen) X_ABSTRACT;
    };

    template<typename T>
    X_INLINE bool ICompressor::deflate(core::MemoryArenaBase* arena, const core::Array<T>& data,
        core::Array<uint8_t>& compressed, CompressLevel::Enum lvl)
    {
        static_assert(compileTime::IsPOD<T>::Value, "T must be a POD type.");

        size_t compressedSize = 0;
        size_t bufSize = requiredDeflateDestBuf(data.size() * sizeof(T));

        compressed.resize(bufSize);

        bool res = deflate_int(arena, data.ptr(), data.size() * sizeof(T),
            compressed.ptr(), compressed.size(), compressedSize, lvl);

        compressed.resize(compressedSize);
        return res;
    }

    template<typename T>
    X_INLINE bool ICompressor::deflate(core::MemoryArenaBase* arena, core::span<const T> data,
        core::Array<uint8_t>& compressed, CompressLevel::Enum lvl)
    {
        static_assert(compileTime::IsPOD<T>::Value, "T must be a POD type.");

        size_t compressedSize = 0;
        size_t numElems = data.size();
        size_t bufSize = requiredDeflateDestBuf(numElems * sizeof(T));

        compressed.resize(bufSize);

        bool res = deflate_int(arena, data.data(), numElems * sizeof(T),
            compressed.ptr(), compressed.size(), compressedSize, lvl);

        compressed.resize(compressedSize);
        return res;
    }

    template<typename T>
    X_INLINE bool ICompressor::inflate(core::MemoryArenaBase* arena, const core::Array<uint8_t>& data, core::Array<T>& inflated)
    {
        static_assert(compileTime::IsPOD<T>::Value, "T must be a POD type.");

        BufferHdr* pHdr = union_cast<BufferHdr*, const uint8_t*>(data.ptr());
        inflated.resize(pHdr->inflatedSize);

        return inflate_int(arena, pHdr + 1, data.size() - sizeof(BufferHdr),
            inflated.ptr(), inflated.size() * sizeof(T));
    }

    template<typename T>
    X_INLINE bool ICompressor::inflate(core::MemoryArenaBase* arena, core::span<const uint8_t> data, core::Array<T>& inflated)
    {
        static_assert(compileTime::IsPOD<T>::Value, "T must be a POD type.");

        size_t dataSize = data.size();

        BufferHdr* pHdr = union_cast<BufferHdr*, const uint8_t*>(data.data());
        inflated.resize(pHdr->inflatedSize);

        return inflate_int(arena, pHdr + 1, dataSize - sizeof(BufferHdr),
            inflated.ptr(), inflated.size() * sizeof(T));
    }

    X_INLINE bool ICompressor::inflate(core::MemoryArenaBase* arena, const uint8_t* pBegin, const uint8_t* pEnd,
        uint8_t* pInflatedBegin, uint8_t* pInflatedEnd)
    {
        size_t dataSize = union_cast<size_t>(pEnd - pBegin);
        size_t dstDataSize = union_cast<size_t>(pInflatedEnd - pInflatedBegin);

        BufferHdr* pHdr = union_cast<BufferHdr*, const uint8_t*>(pBegin);

        X_ASSERT(dstDataSize == pHdr->inflatedSize, "Dest buffer incorrect size")(dstDataSize, pHdr->inflatedSize);

        return inflate_int(arena, pHdr + 1, dataSize - sizeof(BufferHdr), pInflatedBegin, dstDataSize);
    }

    template<typename T>
    struct Compressor : public ICompressor
    {
        using ICompressor::deflate;
        using ICompressor::inflate;

        virtual size_t maxSourceSize(void) const X_FINAL
        {
            return T::maxSourceSize();
        }

        // buffer than source is garanted to fit into.
        virtual size_t requiredDeflateDestBuf(size_t sourceLen) const X_FINAL
        {
            X_ASSERT(sourceLen <= T::maxSourceSize(), "Source len exceeds max source size")(sourceLen, T::maxSourceSize());
            return T::requiredDeflateDestBuf(sourceLen) + sizeof(BufferHdr);
        }

    private:
        virtual bool deflate_int(core::MemoryArenaBase* arena, const void* pSrcBuf, size_t srcBufLen,
            void* pDstBuf, size_t destBufLen, size_t& destLenOut,
            CompressLevel::Enum lvl) X_FINAL
        {
            uint8_t* pDestu8 = reinterpret_cast<uint8_t*>(pDstBuf);

            bool res = T::deflate(arena, pSrcBuf, srcBufLen,
                pDestu8 + sizeof(BufferHdr), destBufLen - sizeof(BufferHdr), destLenOut, lvl);

            if (res) {
                BufferHdr* pHdr = union_cast<BufferHdr*, void*>(pDstBuf);
                pHdr->inflatedSize = safe_static_cast<uint32_t>(srcBufLen);
                pHdr->deflatedSize = safe_static_cast<uint32_t>(destLenOut);
                pHdr->algo = T::getAlgo();
                pHdr->magic[0] = ((BufferHdr::MAGIC & 0xff0000) >> 16) & 0xff;
                pHdr->magic[1] = ((BufferHdr::MAGIC & 0xff00) >> 8) & 0xff;
                pHdr->magic[2] = (BufferHdr::MAGIC & 0xff) & 0xff;
                pHdr->flags.Clear();
                pHdr->sharedDictId = 0;

                // inc hdr in dest len
                destLenOut += sizeof(BufferHdr);
            }

            return res;
        }

        virtual bool inflate_int(core::MemoryArenaBase* arena, const void* pSrcBuf, size_t srcBufLen,
            void* pDstBuf, size_t destBufLen) X_FINAL
        {
            return T::inflate(arena, pSrcBuf, srcBufLen, pDstBuf, destBufLen);
        }
    };

} // namespace Compression

X_NAMESPACE_END

#endif // !_X_COMPRESSION_I_H_
