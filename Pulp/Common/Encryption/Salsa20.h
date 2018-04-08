#pragma once

#ifndef X_ENCRYPTION_SALSA20_H_
#define X_ENCRYPTION_SALSA20_H_

#include <Util\Span.h>

X_NAMESPACE_BEGIN(core)

namespace Encryption
{

    class Salsa20
    {
        static const int32_t NUM_ROUNDS = 12; // 20 is normal but 12 is fine.

    public:
        enum : size_t
        {
            VECTOR_SIZE = 16,
            BLOCK_SIZE = 64,
            KEY_SIZE = 32,
            IV_SIZE = 8
        };

        typedef std::array<uint8_t, KEY_SIZE> Key;
        typedef std::array<uint8_t, IV_SIZE> Iv;

    public:
        Salsa20();
        explicit Salsa20(const Key& key);
        Salsa20(const Salsa20& oth) = default;
        ~Salsa20();

        Salsa20& operator=(const Salsa20&) = default;

        void setKey(const Key& key);
        void setIv(const Iv& iv);

        void processBlocks(const uint8_t* input, uint8_t* output, size_t numBlocks);
        void processBytes(span<const uint8_t> input, span<uint8_t> output);
        void processBytes(span<const uint8_t> input, span<uint8_t> output, int64_t byteOffset);

        template<typename T>
        X_INLINE void processBytes(span<const T> input, span<T> output);
        template<typename T>
        X_INLINE void processBytes(span<const T> input, span<T> output, int64_t byteOffset);

    private:
        void generateKeyStream(uint8_t output[BLOCK_SIZE]);
        void generateKeyStreamSSEOrdering(uint8_t output[BLOCK_SIZE]);

        static bool SSEnabled(void);
        static bool SSESupported(void);

        void processBytesSSE(const uint8_t* input, uint8_t* output,
            size_t numBytes, int64_t byteOffset);

    private:
        inline uint32_t rotate(uint32_t value, uint32_t numBits);
        inline void convert(uint32_t value, uint8_t* array);
        inline uint32_t convert(const uint8_t* array);

        union
        {
            __m128i vectorSSE_[4];
            uint32_t vector_[VECTOR_SIZE];
        };

        static __m128i s_maskLo32;
        static __m128i s_maskHi32;

        struct SSECheckState
        {
            enum Enum
            {
                NOT_CHECKED,
                NOT_SUPPORTED,
                SUPPORTED
            };
        };

        static SSECheckState::Enum s_SSEState_;
        static Spinlock s_checkLock;
    };

    template<typename T>
    X_INLINE void Salsa20::processBytes(span<const T> input, span<T> output)
    {
        static_assert(compileTime::IsPOD<T>::Value, "Encrypt of none POD type");

        processBytes(span<const uint8_t>(reinterpret_cast<const uint8_t*>(input.data()), input.size_bytes()), 
            span<uint8_t>(reinterpret_cast<uint8_t*>(output.data()), output.size_bytes()));
    }

    template<typename T>
    X_INLINE void Salsa20::processBytes(span<const T> input, span<T> output, int64_t byteOffset)
    {
        static_assert(compileTime::IsPOD<T>::Value, "Encrypt of none POD type");

        processBytes(span<const uint8_t>(reinterpret_cast<const uint8_t*>(input.data()), input.size_bytes()),
            span<uint8_t>(reinterpret_cast<uint8_t*>(output.data()), output.size_bytes()), byteOffset);
    }


} // namespace Encryption

X_NAMESPACE_END

#endif // X_ENCRYPTION_SALSA20_H_