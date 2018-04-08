#pragma once

#ifndef X_HASH_MD5_H_
#define X_HASH_MD5_H_

#include "Digest.h"

X_NAMESPACE_BEGIN(core)

namespace Hash
{
    typedef Digest<16> MD5Digest;

    class MD5
    {
        static const uint32_t DIGEST_INTS = 4;
        static const uint32_t DIGEST_BYTES = DIGEST_INTS * 4;
        static const uint32_t BLOCK_INTS = 16;
        static const uint32_t BLOCK_BYTES = BLOCK_INTS * 4;

    public:
        typedef MD5Digest Digest;

        static_assert(Digest::NUM_BYTES == DIGEST_INTS * sizeof(int32_t), "Size mismatch");

    public:
        MD5();
        ~MD5();

        void reset(void);
        void update(const void* pBuf, size_t length);
        void update(const char* pStr);

        template<typename T>
        X_INLINE void update(const T& obj);
        X_INLINE void update(const core::string& str);
        X_INLINE void update(const std::string& str);
        X_INLINE void update(const std::wstring& str);

        Digest& finalize(void);

        // performs init, update and finalize.
        static Digest calc(const void* src, size_t bytelength);

    private:
        void transform(const uint8_t block[BLOCK_BYTES]);
        static void decode(uint32_t* pOutput, const uint8_t* pInput, size_t len);
        static void encode(uint8_t* pOutput, const uint32_t* pInput, size_t len);

    private:
        uint32_t state_[4];
        uint32_t count_[2]; // bits
        uint8_t buffer_[BLOCK_BYTES];

        bool finalized_;

        MD5Digest digest_;
    };

} // namespace Hash

X_NAMESPACE_END

#include "MD5.inl"

#endif // !X_HASH_MD5_H_