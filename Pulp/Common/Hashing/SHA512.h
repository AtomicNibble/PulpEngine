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
        static const uint32_t DIGEST_INTS = 16;
        static const uint32_t DIGEST_BYTES = DIGEST_INTS * 4;
        static const uint32_t BLOCK_INTS = 32;
        static const uint32_t BLOCK_BYTES = BLOCK_INTS * 4;

    public:
        typedef SHA512Digest Digest;

        static_assert(Digest::NUM_BYTES == DIGEST_INTS * sizeof(int32_t), "Size mismatch");

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

        Digest finalize(void);

        // performs init, update and finalize.
        static Digest calc(const void* src, size_t bytelength);

    private:
        void transform(const uint8_t* pBuffer);

        uint8_t buffer_[BLOCK_BYTES];
        uint64_t digest_[DIGEST_INTS / 2];
        uint32_t count_[4];
    };

} // namespace Hash

X_NAMESPACE_END

#include "SHA512.inl"

#endif // X_HASH_SHA512_H_