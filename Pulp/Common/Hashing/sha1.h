#pragma once

#ifndef X_HASH_SHA1_H_
#define X_HASH_SHA1_H_

#include "Digest.h"

X_NAMESPACE_BEGIN(core)

namespace Hash
{
    typedef Digest<20> SHA1Digest;

    class SHA1
    {
        static const uint32_t DIGEST_INTS = 5;
        static const uint32_t DIGEST_BYTES = DIGEST_INTS * 4;
        static const uint32_t BLOCK_INTS = 16;
        static const uint32_t BLOCK_BYTES = BLOCK_INTS * 4;

    public:
        typedef SHA1Digest Digest;

        static_assert(Digest::NUM_BYTES == DIGEST_INTS * sizeof(int32_t), "Size mismatch");

    public:
        SHA1();
        ~SHA1();

        void reset(void);
        void update(const void* pBuf, size_t bytelength);
        void update(const char* pStr);

        template<typename T>
        X_INLINE void update(const T& obj);
        X_INLINE void update(const core::string& str);
        X_INLINE void update(const std::string& str);
        X_INLINE void update(const std::wstring& str);

        Digest finalize(void);

        // performs init, update and finalize.
        static Digest calc(const void* src, size_t bytelength);

    private:
        void transform(const uint8_t* pBuffer);

    private:
        uint8_t buffer_[BLOCK_BYTES];

        union
        {
            uint8_t c[BLOCK_BYTES];
            uint32_t l[BLOCK_INTS];
        } block_;

        uint32_t digest_[DIGEST_INTS];
        size_t numBytes_;
    };

} // namespace Hash

X_NAMESPACE_END

#include "sha1.inl"

#endif // X_HASH_SHA1_H_