#include "EngineCommon.h"
#include "Salsa20.h"

#include <Util\Cpu.h>

X_NAMESPACE_BEGIN(core)

namespace Encryption
{
    namespace
    {
#define ROTATE(v, c) (((v) << (c)) | ((v) >> (32 - (c))))
#define XOR(v, w) ((v) ^ (w))
#define PLUS(v, w) ((uint32_t)((v) + (w)))
// fast 32-bit load/store
#define U8TO32_LITTLE(p) (*((const uint32_t*)((const void*)(p))))
#define U32TO8_LITTLE(c, v) *((uint32_t*)((void*)(c))) = (v)
    } // namespace

#if SALSA20_SSE
    __m128i Salsa20::s_maskLo32;
    __m128i Salsa20::s_maskHi32;
    Spinlock Salsa20::s_checkLock;

#if SWISS_64
    // don't check for x64.
    Salsa20::SSECheckState::Enum Salsa20::s_SSEState_ = SSECheckState::SUPPORTED;
#else
    Salsa20::SSECheckState::Enum Salsa20::s_SSEState_ = SSECheckState::NOT_CHECKED;
#endif // !SWISS_64

#endif // SALSA20_SSE

    Salsa20::Salsa20(void)
    {
        core::zero_object(vector_);
    }

    Salsa20::Salsa20(const uint8_t* pKey)
    {
        X_ASSERT_NOT_NULL(pKey);
        core::zero_object(vector_);
        setKey(pKey);
    }

    Salsa20::Salsa20(const Salsa20& oth)
    {
        ::memcpy(vector_, oth.vector_, sizeof(vector_));
    }

    Salsa20::~Salsa20()
    {
    }

    //----------------------------------------------------------------------------------
    void Salsa20::setKey(const uint8_t* key)
    {
        static const char constant_str[] = "expand 32-byte k";

        if (key == nullptr)
            return;

        if (SSEnabled()) {
            const uint32_t* key32 = reinterpret_cast<const uint32_t*>(key);

            vector_[0] = convert(reinterpret_cast<const uint8_t*>(&constant_str[0]));
            ; // 0
            vector_[3] = convert(reinterpret_cast<const uint8_t*>(&constant_str[12]));
            ; // 12
            vector_[13] = key32[0];
            vector_[10] = key32[1];
            vector_[7] = key32[2];
            vector_[4] = key32[3];
            vector_[1] = convert(reinterpret_cast<const uint8_t*>(&constant_str[4]));
            ; // 4
            vector_[2] = convert(reinterpret_cast<const uint8_t*>(&constant_str[8]));
            ; // 8
            vector_[15] = key32[4];
            vector_[12] = key32[5];
            vector_[9] = key32[6];
            vector_[6] = key32[7];

            vector_[14] = 0;
            vector_[11] = 0;
            vector_[5] = 0;
            vector_[8] = 0;
        }
        else {
            vector_[0] = convert(reinterpret_cast<const uint8_t*>(&constant_str[0]));
            vector_[1] = convert(&key[0]);
            vector_[2] = convert(&key[4]);
            vector_[3] = convert(&key[8]);
            vector_[4] = convert(&key[12]);
            vector_[5] = convert(reinterpret_cast<const uint8_t*>(&constant_str[4]));

            memset(&vector_[6], 0, 4 * sizeof(uint32_t));

            vector_[10] = convert(reinterpret_cast<const uint8_t*>(&constant_str[8]));
            vector_[11] = convert(&key[16]);
            vector_[12] = convert(&key[20]);
            vector_[13] = convert(&key[24]);
            vector_[14] = convert(&key[28]);
            vector_[15] = convert(reinterpret_cast<const uint8_t*>(&constant_str[12]));
        }
    }

    //----------------------------------------------------------------------------------
    void Salsa20::setIv(const uint8_t* iv)
    {
        if (iv == nullptr)
            return;

        if (SSEnabled()) {
            const uint32_t* iv32 = reinterpret_cast<const uint32_t*>(iv);

            vector_[14] = iv32[0];
            vector_[11] = iv32[1];
            vector_[8] = 0;
            vector_[5] = 0;
        }
        else {
            vector_[6] = convert(&iv[0]);
            vector_[7] = convert(&iv[4]);
            vector_[8] = 0;
            vector_[9] = 0;
        }
    }

    //----------------------------------------------------------------------------------
    void Salsa20::generateKeyStream(uint8_t pOutput[BLOCK_SIZE])
    {
        uint32_t x[VECTOR_SIZE];
        memcpy(x, vector_, sizeof(vector_));

        for (int32_t i = 20; i > 0; i -= 2) {
            x[4] ^= rotate(static_cast<uint32_t>(x[0] + x[12]), 7);
            x[8] ^= rotate(static_cast<uint32_t>(x[4] + x[0]), 9);
            x[12] ^= rotate(static_cast<uint32_t>(x[8] + x[4]), 13);
            x[0] ^= rotate(static_cast<uint32_t>(x[12] + x[8]), 18);
            x[9] ^= rotate(static_cast<uint32_t>(x[5] + x[1]), 7);
            x[13] ^= rotate(static_cast<uint32_t>(x[9] + x[5]), 9);
            x[1] ^= rotate(static_cast<uint32_t>(x[13] + x[9]), 13);
            x[5] ^= rotate(static_cast<uint32_t>(x[1] + x[13]), 18);
            x[14] ^= rotate(static_cast<uint32_t>(x[10] + x[6]), 7);
            x[2] ^= rotate(static_cast<uint32_t>(x[14] + x[10]), 9);
            x[6] ^= rotate(static_cast<uint32_t>(x[2] + x[14]), 13);
            x[10] ^= rotate(static_cast<uint32_t>(x[6] + x[2]), 18);
            x[3] ^= rotate(static_cast<uint32_t>(x[15] + x[11]), 7);
            x[7] ^= rotate(static_cast<uint32_t>(x[3] + x[15]), 9);
            x[11] ^= rotate(static_cast<uint32_t>(x[7] + x[3]), 13);
            x[15] ^= rotate(static_cast<uint32_t>(x[11] + x[7]), 18);
            x[1] ^= rotate(static_cast<uint32_t>(x[0] + x[3]), 7);
            x[2] ^= rotate(static_cast<uint32_t>(x[1] + x[0]), 9);
            x[3] ^= rotate(static_cast<uint32_t>(x[2] + x[1]), 13);
            x[0] ^= rotate(static_cast<uint32_t>(x[3] + x[2]), 18);
            x[6] ^= rotate(static_cast<uint32_t>(x[5] + x[4]), 7);
            x[7] ^= rotate(static_cast<uint32_t>(x[6] + x[5]), 9);
            x[4] ^= rotate(static_cast<uint32_t>(x[7] + x[6]), 13);
            x[5] ^= rotate(static_cast<uint32_t>(x[4] + x[7]), 18);
            x[11] ^= rotate(static_cast<uint32_t>(x[10] + x[9]), 7);
            x[8] ^= rotate(static_cast<uint32_t>(x[11] + x[10]), 9);
            x[9] ^= rotate(static_cast<uint32_t>(x[8] + x[11]), 13);
            x[10] ^= rotate(static_cast<uint32_t>(x[9] + x[8]), 18);
            x[12] ^= rotate(static_cast<uint32_t>(x[15] + x[14]), 7);
            x[13] ^= rotate(static_cast<uint32_t>(x[12] + x[15]), 9);
            x[14] ^= rotate(static_cast<uint32_t>(x[13] + x[12]), 13);
            x[15] ^= rotate(static_cast<uint32_t>(x[14] + x[13]), 18);
        }

        for (size_t i = 0; i < VECTOR_SIZE; ++i) {
            x[i] += vector_[i];
        }

        for (size_t i = 0; i < VECTOR_SIZE; ++i) {
            convert(x[i], &pOutput[4 * i]);
        }

        ++vector_[8];
        vector_[9] += vector_[8] == 0 ? 1 : 0;
    }

#if SALSA20_SSE

    // This is not very efficent but not important as it's only used to gernerate one keystream
    // only if the byte offset of the incomming buffer is not a multiple of block size.
    // before the buffer is passed to the sse version.
    void Salsa20::generateKeyStreamSSEOrdering(uint8_t pOutput[BLOCK_SIZE])
    {
        uint32_t x[VECTOR_SIZE];

        // 0 - 0
        x[0] = vector_[0];
        // 1 - 13
        x[1] = vector_[13];
        // 2 -10
        x[2] = vector_[10];
        // 3 - 7
        x[3] = vector_[7];
        // 4 - 4
        x[4] = vector_[4];
        // 5 - 1
        x[5] = vector_[1];

        // IV
        // 6 - 14
        x[6] = vector_[14];
        // 7 - 11
        x[7] = vector_[11];
        // 8 - 5
        x[8] = vector_[8];
        // 9 - 8
        x[9] = vector_[5];

        // 10 - 2
        x[10] = vector_[2];
        // 11 - 15
        x[11] = vector_[15];
        // 12 - 12
        x[12] = vector_[12];
        // 13 - 9
        x[13] = vector_[9];
        // 14 - 6
        x[14] = vector_[6];
        // 15 - 3
        x[15] = vector_[3];

        for (int32_t i = NUM_ROUNDS; i > 0; i -= 2) {
            x[4] ^= rotate(static_cast<uint32_t>(x[0] + x[12]), 7);
            x[8] ^= rotate(static_cast<uint32_t>(x[4] + x[0]), 9);
            x[12] ^= rotate(static_cast<uint32_t>(x[8] + x[4]), 13);
            x[0] ^= rotate(static_cast<uint32_t>(x[12] + x[8]), 18);
            x[9] ^= rotate(static_cast<uint32_t>(x[5] + x[1]), 7);
            x[13] ^= rotate(static_cast<uint32_t>(x[9] + x[5]), 9);
            x[1] ^= rotate(static_cast<uint32_t>(x[13] + x[9]), 13);
            x[5] ^= rotate(static_cast<uint32_t>(x[1] + x[13]), 18);
            x[14] ^= rotate(static_cast<uint32_t>(x[10] + x[6]), 7);
            x[2] ^= rotate(static_cast<uint32_t>(x[14] + x[10]), 9);
            x[6] ^= rotate(static_cast<uint32_t>(x[2] + x[14]), 13);
            x[10] ^= rotate(static_cast<uint32_t>(x[6] + x[2]), 18);
            x[3] ^= rotate(static_cast<uint32_t>(x[15] + x[11]), 7);
            x[7] ^= rotate(static_cast<uint32_t>(x[3] + x[15]), 9);
            x[11] ^= rotate(static_cast<uint32_t>(x[7] + x[3]), 13);
            x[15] ^= rotate(static_cast<uint32_t>(x[11] + x[7]), 18);
            x[1] ^= rotate(static_cast<uint32_t>(x[0] + x[3]), 7);
            x[2] ^= rotate(static_cast<uint32_t>(x[1] + x[0]), 9);
            x[3] ^= rotate(static_cast<uint32_t>(x[2] + x[1]), 13);
            x[0] ^= rotate(static_cast<uint32_t>(x[3] + x[2]), 18);
            x[6] ^= rotate(static_cast<uint32_t>(x[5] + x[4]), 7);
            x[7] ^= rotate(static_cast<uint32_t>(x[6] + x[5]), 9);
            x[4] ^= rotate(static_cast<uint32_t>(x[7] + x[6]), 13);
            x[5] ^= rotate(static_cast<uint32_t>(x[4] + x[7]), 18);
            x[11] ^= rotate(static_cast<uint32_t>(x[10] + x[9]), 7);
            x[8] ^= rotate(static_cast<uint32_t>(x[11] + x[10]), 9);
            x[9] ^= rotate(static_cast<uint32_t>(x[8] + x[11]), 13);
            x[10] ^= rotate(static_cast<uint32_t>(x[9] + x[8]), 18);
            x[12] ^= rotate(static_cast<uint32_t>(x[15] + x[14]), 7);
            x[13] ^= rotate(static_cast<uint32_t>(x[12] + x[15]), 9);
            x[14] ^= rotate(static_cast<uint32_t>(x[13] + x[12]), 13);
            x[15] ^= rotate(static_cast<uint32_t>(x[14] + x[13]), 18);
        }

        x[0] += vector_[0];
        x[1] += vector_[13];
        x[2] += vector_[10];
        x[3] += vector_[7];
        x[4] += vector_[4];
        x[5] += vector_[1];

        // IV
        x[6] += vector_[14];
        x[7] += vector_[11];
        x[8] += vector_[8];
        x[9] += vector_[5];

        x[10] += vector_[2];
        x[11] += vector_[15];
        x[12] += vector_[12];
        x[13] += vector_[9];
        x[14] += vector_[6];
        x[15] += vector_[3];

        for (size_t i = 0; i < VECTOR_SIZE; ++i) {
            convert(x[i], &pOutput[4 * i]);
        }

        ++vector_[8];
        vector_[5] += vector_[8] == 0 ? 1 : 0;
    }
#endif // SALSA20_SSE

    //----------------------------------------------------------------------------------
    void Salsa20::processBlocks(const uint8_t* pInput, uint8_t* pOutput, size_t numBlocks)
    {
        X_ASSERT_NOT_NULL(pInput);
        X_ASSERT_NOT_NULL(pOutput);

#if SALSA20_SSE
        if (SSEnabled()) {
            const int64_t byteOffset = ((vector_[5] * 0xFFFFFFFF) + vector_[8]) * BLOCK_SIZE;

            processBytesSSE(pInput, pOutput, numBlocks * BLOCK_SIZE, byteOffset);
            return;
        }
#endif // !SALSA20_SSE

        uint8_t keyStream[BLOCK_SIZE];

        for (size_t i = 0; i < numBlocks; ++i) {
            generateKeyStream(keyStream);

            for (size_t j = 0; j < BLOCK_SIZE; ++j) {
                *(pOutput++) = keyStream[j] ^ *(pInput++);
            }
        }
    }

    //----------------------------------------------------------------------------------
    void Salsa20::processBytes(const uint8_t* pInput, uint8_t* pOutput, size_t numBytes)
    {
        X_ASSERT_NOT_NULL(pInput);
        X_ASSERT_NOT_NULL(pOutput);

#if SALSA20_SSE
        if (SSEnabled()) {
            // work out current byte offset to pass to sse version.
            const int64_t byteOffset = ((vector_[5] * 0xFFFFFFFF) + vector_[8]) * BLOCK_SIZE;

            processBytesSSE(pInput, pOutput, numBytes, byteOffset);
            return;
        }
#endif // !SALSA20_SSE

        uint8_t keyStream[BLOCK_SIZE];
        size_t numBytesToProcess;

        while (numBytes != 0) {
            generateKeyStream(keyStream);
            numBytesToProcess = numBytes >= BLOCK_SIZE ? BLOCK_SIZE : numBytes;

            for (size_t i = 0; i < numBytesToProcess; ++i, --numBytes) {
                *(pOutput++) = keyStream[i] ^ *(pInput++);
            }
        }
    }

    //----------------------------------------------------------------------------------
    void Salsa20::processBytes(const uint8_t* pInput, uint8_t* pOutput,
        size_t numBytes, int64_t byteOffset)
    {
        uint8_t keyStream[BLOCK_SIZE];
        size_t streamOffset = byteOffset % BLOCK_SIZE;
        size_t numBytesToProcess;

#if SALSA20_SSE
        if (SSEnabled()) {
            if (streamOffset > 0) {
                // update iv for this stream offset block.
                int64_t blockOffset = (byteOffset / BLOCK_SIZE);
                vector_[8] = blockOffset & 0xFFFFFFFF;
                vector_[5] = (blockOffset / 0xFFFFFFFF) & 0xFFFFFFF;

                generateKeyStreamSSEOrdering(keyStream);
                numBytesToProcess = core::Min(numBytes, BLOCK_SIZE - streamOffset);

                for (size_t i = streamOffset; i < streamOffset + numBytesToProcess; ++i, --numBytes) {
                    *(pOutput++) = keyStream[i] ^ *(pInput++);
                }

                // patch vars since we reset iv coutns in processBytesSSE
                byteOffset += numBytesToProcess;
            }

            processBytesSSE(pInput, pOutput, numBytes, byteOffset);
            return;
        }
#endif // !SALSA20_SSE

        // supports decrypting from any offset.
        // we will need to update the IV to the block offset.
        // and offset into the keyStream if not a block aligned offset.
        int64_t blockOffset = (byteOffset / BLOCK_SIZE);

        vector_[8] = blockOffset & 0xFFFFFFFF;
        vector_[9] = (blockOffset / 0xFFFFFFFF) & 0xFFFFFFF;

        if (streamOffset > 0) {
            generateKeyStream(keyStream);
            numBytesToProcess = core::Min(numBytes, BLOCK_SIZE - streamOffset);

            for (size_t i = streamOffset; i < streamOffset + numBytesToProcess; ++i, --numBytes) {
                *(pOutput++) = keyStream[i] ^ *(pInput++);
            }
        }

        while (numBytes != 0) {
            generateKeyStream(keyStream);
            numBytesToProcess = numBytes >= BLOCK_SIZE ? BLOCK_SIZE : numBytes;

            for (size_t i = 0; i < numBytesToProcess; ++i, --numBytes) {
                *(pOutput++) = keyStream[i] ^ *(pInput++);
            }
        }
    }

    //----------------------------------------------------------------------------------
    bool Salsa20::SSEnabled(void)
    {
#if SALSA20_SSE
        if (s_SSEState_ == SSECheckState::SUPPORTED) {
            return true;
        }

        if (s_SSEState_ == SSECheckState::NOT_SUPPORTED) {
            return false;
        }

        // check.
        Spinlock::ScopedLock lock(s_checkLock);
        // check after lock, incase another thread init it.
        if (s_SSEState_ == SSECheckState::NOT_CHECKED) {
            // i'll add in a proper sse cpu class first.
            if (SSESupported()) {
                s_maskLo32 = _mm_shuffle_epi32(_mm_cvtsi32_si128(-1), _MM_SHUFFLE(1, 0, 1, 0));
                s_maskHi32 = _mm_slli_epi64(s_maskLo32, 32);
                s_SSEState_ = SSECheckState::SUPPORTED;
            }
            else {
                s_SSEState_ = SSECheckState::NOT_SUPPORTED;
            }
        }

        return s_SSEState_ == SSECheckState::SUPPORTED;
#else
        return false;
#endif // ! SALSA20_SSE
    }

#if SALSA20_SSE

    //----------------------------------------------------------------------------------
    bool Salsa20::SSESupported(void)
    {
        CpuInfo cpuInfo;

        const CpuInfo::CpuID::Info1& info1 = cpuInfo.GetInfoType1();
        if (info1.edx.SSE2_) {
            return true;
        }
        X_WARNING("Salsa20", "SEE2 not supported faling back to none sse version.");
        return false;
    }

    //----------------------------------------------------------------------------------
    void Salsa20::processBytesSSE(const uint8_t* pInput, uint8_t* pOutput,
        size_t numBytes, int64_t byteOffset)
    {
        uint8_t keyStream[BLOCK_SIZE];
        size_t bytes = numBytes;
        size_t i;

        const uint8_t* m = pInput;
        uint8_t* c = pOutput;
        uint8_t* ctarget = c;

        {
            // counts are 8 & 5
            int64_t blockOffset = (byteOffset / BLOCK_SIZE);

            vector_[8] = blockOffset & 0xFFFFFFFF;
            vector_[5] = (blockOffset / 0xFFFFFFFF) & 0xFFFFFFF;
        }

        for (;;) {
            if (bytes < BLOCK_SIZE) {
                for (i = 0; i < bytes; ++i) {
                    keyStream[i] = m[i];
                }

                m = keyStream;
                ctarget = c;
                c = keyStream;
            }

            __m128i X0 = _mm_loadu_si128((const __m128i*)&(vectorSSE_[0]));
            __m128i X1 = _mm_loadu_si128((const __m128i*)&(vectorSSE_[1]));
            __m128i X2 = _mm_loadu_si128((const __m128i*)&(vectorSSE_[2]));
            __m128i X3 = _mm_loadu_si128((const __m128i*)&(vectorSSE_[3]));
            __m128i X0s = X0;
            __m128i X1s = X1;
            __m128i X2s = X2;
            __m128i X3s = X3;

            for (i = 0; i < NUM_ROUNDS / 2; ++i) {
                __m128i T = _mm_add_epi32(X0, X3);
                X1 = _mm_xor_si128(X1, _mm_slli_epi32(T, 7));
                X1 = _mm_xor_si128(X1, _mm_srli_epi32(T, 25));
                T = _mm_add_epi32(X1, X0);
                X2 = _mm_xor_si128(X2, _mm_slli_epi32(T, 9));
                X2 = _mm_xor_si128(X2, _mm_srli_epi32(T, 23));
                T = _mm_add_epi32(X2, X1);
                X3 = _mm_xor_si128(X3, _mm_slli_epi32(T, 13));
                X3 = _mm_xor_si128(X3, _mm_srli_epi32(T, 19));
                T = _mm_add_epi32(X3, X2);
                X0 = _mm_xor_si128(X0, _mm_slli_epi32(T, 18));
                X0 = _mm_xor_si128(X0, _mm_srli_epi32(T, 14));

                X1 = _mm_shuffle_epi32(X1, 0x93);
                X2 = _mm_shuffle_epi32(X2, 0x4E);
                X3 = _mm_shuffle_epi32(X3, 0x39);

                T = _mm_add_epi32(X0, X1);
                X3 = _mm_xor_si128(X3, _mm_slli_epi32(T, 7));
                X3 = _mm_xor_si128(X3, _mm_srli_epi32(T, 25));
                T = _mm_add_epi32(X3, X0);
                X2 = _mm_xor_si128(X2, _mm_slli_epi32(T, 9));
                X2 = _mm_xor_si128(X2, _mm_srli_epi32(T, 23));
                T = _mm_add_epi32(X2, X3);
                X1 = _mm_xor_si128(X1, _mm_slli_epi32(T, 13));
                X1 = _mm_xor_si128(X1, _mm_srli_epi32(T, 19));
                T = _mm_add_epi32(X1, X2);
                X0 = _mm_xor_si128(X0, _mm_slli_epi32(T, 18));
                X0 = _mm_xor_si128(X0, _mm_srli_epi32(T, 14));

                X1 = _mm_shuffle_epi32(X1, 0x39);
                X2 = _mm_shuffle_epi32(X2, 0x4E);
                X3 = _mm_shuffle_epi32(X3, 0x93);
            }

            X0 = _mm_add_epi32(X0s, X0);
            X1 = _mm_add_epi32(X1s, X1);
            X2 = _mm_add_epi32(X2s, X2);
            X3 = _mm_add_epi32(X3s, X3);

            {
                __m128i k02 = _mm_or_si128(_mm_slli_epi64(X0, 32), _mm_srli_epi64(X3, 32));
                k02 = _mm_shuffle_epi32(k02, _MM_SHUFFLE(0, 1, 2, 3));
                __m128i k13 = _mm_or_si128(_mm_slli_epi64(X1, 32), _mm_srli_epi64(X0, 32));
                k13 = _mm_shuffle_epi32(k13, _MM_SHUFFLE(0, 1, 2, 3));
                __m128i k20 = _mm_or_si128(_mm_and_si128(X2, s_maskLo32), _mm_and_si128(X1, s_maskHi32));
                __m128i k31 = _mm_or_si128(_mm_and_si128(X3, s_maskLo32), _mm_and_si128(X2, s_maskHi32));

                const float* const mv = reinterpret_cast<const float*>(m);
                float* const cv = reinterpret_cast<float*>(c);

                _mm_storeu_ps(cv, _mm_castsi128_ps(_mm_xor_si128(_mm_unpackhi_epi64(k02, k20), _mm_castps_si128(_mm_loadu_ps(mv)))));
                _mm_storeu_ps(cv + 4, _mm_castsi128_ps(_mm_xor_si128(_mm_unpackhi_epi64(k13, k31), _mm_castps_si128(_mm_loadu_ps(mv + 4)))));
                _mm_storeu_ps(cv + 8, _mm_castsi128_ps(_mm_xor_si128(_mm_unpacklo_epi64(k20, k02), _mm_castps_si128(_mm_loadu_ps(mv + 8)))));
                _mm_storeu_ps(cv + 12, _mm_castsi128_ps(_mm_xor_si128(_mm_unpacklo_epi64(k31, k13), _mm_castps_si128(_mm_loadu_ps(mv + 12)))));
            }

            if (!(++vector_[8])) {
                ++vector_[5]; // state reordered for SSE
                              // stopping at 2^70 bytes per nonce is user's responsibility
            }

            if (bytes <= BLOCK_SIZE) {
                if (bytes < BLOCK_SIZE) {
                    for (i = 0; i < bytes; ++i) {
                        ctarget[i] = c[i];
                    }
                }
                return;
            }

            bytes -= BLOCK_SIZE;
            c += BLOCK_SIZE;
            m += BLOCK_SIZE;
        }
    }
#endif // SALSA20_SSE

    //----------------------------------------------------------------------------------
    uint32_t Salsa20::rotate(uint32_t value, uint32_t numBits)
    {
        return (value << numBits) | (value >> (32 - numBits));
    }

    //----------------------------------------------------------------------------------
    void Salsa20::convert(uint32_t value, uint8_t* array)
    {
        array[0] = static_cast<uint8_t>((value >> 0) & 0xFF);
        array[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
        array[2] = static_cast<uint8_t>((value >> 16) & 0xFF);
        array[3] = static_cast<uint8_t>((value >> 24) & 0xFF);
    }

    //----------------------------------------------------------------------------------
    uint32_t Salsa20::convert(const uint8_t* array)
    {
        return ((static_cast<uint32_t>(array[0]) << 0) | (static_cast<uint32_t>(array[1]) << 8) | (static_cast<uint32_t>(array[2]) << 16) | (static_cast<uint32_t>(array[3]) << 24));
    }

} // namespace Encryption

X_NAMESPACE_END
