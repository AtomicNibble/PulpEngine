#pragma once

#ifndef X_HASH_SHA512_H_
#define X_HASH_SHA512_H_

#include "Digest.h"

X_NAMESPACE_BEGIN(core)

namespace Hash
{
    typedef Digest<64> SHA512Digest;

    class SHA512
    {
    public:
        typedef SHA512Digest Digest;

    public:
        SHA512();
        ~SHA512();

        void reset(void);
        void update(const void* buf, size_t length);
        void update(const char* str);

        template<typename T>
        X_INLINE void update(const T& obj);
        X_INLINE void update(const core::string& str);
        X_INLINE void update(const std::string& str);
        X_INLINE void update(const std::wstring& str);

        SHA512Digest finalize(void);

        // performs init, update and finalize.
        static void calc(const void* src, const size_t bytelength, SHA512Digest& hash);

    private:
        static const uint32_t DIGEST_INTS = 16;
        static const uint32_t DIGEST_BYTES = DIGEST_INTS * 4;
        static const uint32_t BLOCK_INTS = 32;
        static const uint32_t BLOCK_BYTES = BLOCK_INTS * 4;

    private:
        void transform(const uint8_t* pBuffer);

        uint8_t buffer_[BLOCK_BYTES];
        uint64_t digest_[DIGEST_INTS / 2];
        uint32_t count_[4];
    };

#include "SHA512.inl"
} // namespace Hash

X_NAMESPACE_END

#endif // X_HASH_SHA512_H_