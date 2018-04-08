#include "EngineCommon.h"
#include "SHA512.h"

X_NAMESPACE_BEGIN(core)

namespace Hash
{
    inline uint64_t Ch(uint64_t x, uint64_t y, uint64_t z)
    {
        return z ^ (x & (y ^ z));
    }

    inline uint64_t Maj(uint64_t x, uint64_t y, uint64_t z)
    {
        return (x & y) | (z & (x | y));
    }

    inline uint64_t RORu64(uint64_t x, uint64_t y)
    {
        return (x >> y) | (x << (64 - y));
    }

#define e0(x) (RORu64(x, 28) ^ RORu64(x, 34) ^ RORu64(x, 39))
#define e1(x) (RORu64(x, 14) ^ RORu64(x, 18) ^ RORu64(x, 41))
#define s0(x) (RORu64(x, 1) ^ RORu64(x, 8) ^ (x >> 7))
#define s1(x) (RORu64(x, 19) ^ RORu64(x, 61) ^ (x >> 6))

    inline void LOAD_OP(int I, uint64_t* W, const uint8_t* input)
    {
        uint64_t t1 = input[(8 * I)] & 0xff;
        t1 <<= 8;
        t1 |= input[(8 * I) + 1] & 0xff;
        t1 <<= 8;
        t1 |= input[(8 * I) + 2] & 0xff;
        t1 <<= 8;
        t1 |= input[(8 * I) + 3] & 0xff;
        t1 <<= 8;
        t1 |= input[(8 * I) + 4] & 0xff;
        t1 <<= 8;
        t1 |= input[(8 * I) + 5] & 0xff;
        t1 <<= 8;
        t1 |= input[(8 * I) + 6] & 0xff;
        t1 <<= 8;
        t1 |= input[(8 * I) + 7] & 0xff;
        W[I] = t1;
    }

    inline void BLEND_OP(int I, uint64_t* W)
    {
        W[I] = s1(W[I - 2]) + W[I - 7] + s0(W[I - 15]) + W[I - 16];
    }

    const uint64_t sha512_k[80] = {
        0x428a2f98d728ae22ULL,
        0x7137449123ef65cdULL,
        0xb5c0fbcfec4d3b2fULL,
        0xe9b5dba58189dbbcULL,
        0x3956c25bf348b538ULL,
        0x59f111f1b605d019ULL,
        0x923f82a4af194f9bULL,
        0xab1c5ed5da6d8118ULL,
        0xd807aa98a3030242ULL,
        0x12835b0145706fbeULL,
        0x243185be4ee4b28cULL,
        0x550c7dc3d5ffb4e2ULL,
        0x72be5d74f27b896fULL,
        0x80deb1fe3b1696b1ULL,
        0x9bdc06a725c71235ULL,
        0xc19bf174cf692694ULL,
        0xe49b69c19ef14ad2ULL,
        0xefbe4786384f25e3ULL,
        0x0fc19dc68b8cd5b5ULL,
        0x240ca1cc77ac9c65ULL,
        0x2de92c6f592b0275ULL,
        0x4a7484aa6ea6e483ULL,
        0x5cb0a9dcbd41fbd4ULL,
        0x76f988da831153b5ULL,
        0x983e5152ee66dfabULL,
        0xa831c66d2db43210ULL,
        0xb00327c898fb213fULL,
        0xbf597fc7beef0ee4ULL,
        0xc6e00bf33da88fc2ULL,
        0xd5a79147930aa725ULL,
        0x06ca6351e003826fULL,
        0x142929670a0e6e70ULL,
        0x27b70a8546d22ffcULL,
        0x2e1b21385c26c926ULL,
        0x4d2c6dfc5ac42aedULL,
        0x53380d139d95b3dfULL,
        0x650a73548baf63deULL,
        0x766a0abb3c77b2a8ULL,
        0x81c2c92e47edaee6ULL,
        0x92722c851482353bULL,
        0xa2bfe8a14cf10364ULL,
        0xa81a664bbc423001ULL,
        0xc24b8b70d0f89791ULL,
        0xc76c51a30654be30ULL,
        0xd192e819d6ef5218ULL,
        0xd69906245565a910ULL,
        0xf40e35855771202aULL,
        0x106aa07032bbd1b8ULL,
        0x19a4c116b8d2d0c8ULL,
        0x1e376c085141ab53ULL,
        0x2748774cdf8eeb99ULL,
        0x34b0bcb5e19b48a8ULL,
        0x391c0cb3c5c95a63ULL,
        0x4ed8aa4ae3418acbULL,
        0x5b9cca4f7763e373ULL,
        0x682e6ff3d6b2b8a3ULL,
        0x748f82ee5defb2fcULL,
        0x78a5636f43172f60ULL,
        0x84c87814a1f0ab72ULL,
        0x8cc702081a6439ecULL,
        0x90befffa23631e28ULL,
        0xa4506cebde82bde9ULL,
        0xbef9a3f7b2c67915ULL,
        0xc67178f2e372532bULL,
        0xca273eceea26619cULL,
        0xd186b8c721c0c207ULL,
        0xeada7dd6cde0eb1eULL,
        0xf57d4f7fee6ed178ULL,
        0x06f067aa72176fbaULL,
        0x0a637dc5a2c898a6ULL,
        0x113f9804bef90daeULL,
        0x1b710b35131c471bULL,
        0x28db77f523047d84ULL,
        0x32caab7b40c72493ULL,
        0x3c9ebe0a15c9bebcULL,
        0x431d67c49c100d4cULL,
        0x4cc5d4becb3e42b6ULL,
        0x597f299cfc657e2aULL,
        0x5fcb6fab3ad6faecULL,
        0x6c44198c4a475817ULL,
    };

    // ---------------------------------------------------------

    SHA512::SHA512()
    {
        reset();
    }

    SHA512::~SHA512()
    {
    }

    void SHA512::reset(void)
    {
        zero_object(buffer_);
        zero_object(count_);

        digest_[0] = 0x6a09e667f3bcc908ULL;
        digest_[1] = 0xbb67ae8584caa73bULL;
        digest_[2] = 0x3c6ef372fe94f82bULL;
        digest_[3] = 0xa54ff53a5f1d36f1ULL;
        digest_[4] = 0x510e527fade682d1ULL;
        digest_[5] = 0x9b05688c2b3e6c1fULL;
        digest_[6] = 0x1f83d9abfb41bd6bULL;
        digest_[7] = 0x5be0cd19137e2179ULL;
    }

    void SHA512::update(const void* buf, size_t length)
    {
        size_t index = static_cast<size_t>((count_[0] >> 3) & 0x7F);
        size_t firstpart = BLOCK_BYTES - index;
        size_t i;

        const uint8_t* input = reinterpret_cast<const uint8_t*>(buf);

        if (length >= firstpart) {
            memcpy(&buffer_[index], input, firstpart);
            transform(buffer_);

            for (i = firstpart; i + BLOCK_BYTES <= length; i += BLOCK_BYTES) {
                transform(&input[i]);
            }

            index = 0;
        }
        else {
            i = 0;
        }

        // Update number of bits
        if ((count_[0] += static_cast<uint32_t>(length << 3)) < (length << 3)) {
            if ((count_[1] += 1) < 1) {
                if ((count_[2] += 1) < 1) {
                    count_[3]++;
                }
            }
            count_[1] += static_cast<uint32_t>(length >> 29);
        }

        memcpy(&buffer_[index], &input[i], length - i);
    }

    void SHA512::update(const char* str)
    {
        update(reinterpret_cast<const void*>(str), ::strlen(str));
    }

    SHA512Digest SHA512::finalize(void)
    {
        SHA512Digest res;
        static uint8_t padding[128] = {
            0x80,
        };

        // calculate bits pre pad.
        uint32_t bits[4];
        bits[3] = core::Endian::swap(count_[0]);
        bits[2] = core::Endian::swap(count_[1]);
        bits[1] = core::Endian::swap(count_[2]);
        bits[0] = core::Endian::swap(count_[3]);

        // sha-1: pad out to 56 mod 64.
        // sha-512: Pad out to 112 mod 128.
        size_t index = (count_[0] >> 3) & 0x7F;
        size_t padLen = (index < 112) ? (112 - index) : ((128 + 112) - index);
        update(padding, padLen);
        update(bits, 16);

        for (size_t i = 0; i < DIGEST_BYTES; ++i) {
            uint64_t val = digest_[i >> 3];
            size_t shift = ((7 - (i & 7)) * 8);

            res.bytes[i] = static_cast<uint8_t>((val >> shift) & 0xFF);
        }

        reset();
        return res;
    }

    void SHA512::transform(const uint8_t* pBuffer)
    {
        uint64_t a, b, c, d, e, f, g, h, t1, t2;
        uint64_t W[80];

        int i;

        // load the input
        for (i = 0; i < 16; i++)
            LOAD_OP(i, W, pBuffer);

        for (i = 16; i < 80; i++) {
            BLEND_OP(i, W);
        }

        // load the state into our registers
        a = digest_[0];
        b = digest_[1];
        c = digest_[2];
        d = digest_[3];
        e = digest_[4];
        f = digest_[5];
        g = digest_[6];
        h = digest_[7];

        // now iterate
        for (i = 0; i < 80; i += 8) {
            t1 = h + e1(e) + Ch(e, f, g) + sha512_k[i] + W[i];
            t2 = e0(a) + Maj(a, b, c);
            d += t1;
            h = t1 + t2;
            t1 = g + e1(d) + Ch(d, e, f) + sha512_k[i + 1] + W[i + 1];
            t2 = e0(h) + Maj(h, a, b);
            c += t1;
            g = t1 + t2;
            t1 = f + e1(c) + Ch(c, d, e) + sha512_k[i + 2] + W[i + 2];
            t2 = e0(g) + Maj(g, h, a);
            b += t1;
            f = t1 + t2;
            t1 = e + e1(b) + Ch(b, c, d) + sha512_k[i + 3] + W[i + 3];
            t2 = e0(f) + Maj(f, g, h);
            a += t1;
            e = t1 + t2;
            t1 = d + e1(a) + Ch(a, b, c) + sha512_k[i + 4] + W[i + 4];
            t2 = e0(e) + Maj(e, f, g);
            h += t1;
            d = t1 + t2;
            t1 = c + e1(h) + Ch(h, a, b) + sha512_k[i + 5] + W[i + 5];
            t2 = e0(d) + Maj(d, e, f);
            g += t1;
            c = t1 + t2;
            t1 = b + e1(g) + Ch(g, h, a) + sha512_k[i + 6] + W[i + 6];
            t2 = e0(c) + Maj(c, d, e);
            f += t1;
            b = t1 + t2;
            t1 = a + e1(f) + Ch(f, g, h) + sha512_k[i + 7] + W[i + 7];
            t2 = e0(b) + Maj(b, c, d);
            e += t1;
            a = t1 + t2;
        }

        digest_[0] += a;
        digest_[1] += b;
        digest_[2] += c;
        digest_[3] += d;
        digest_[4] += e;
        digest_[5] += f;
        digest_[6] += g;
        digest_[7] += h;

        // erase our data
        a = b = c = d = e = f = g = h = t1 = t2 = 0;
        memset(W, 0, 80 * sizeof(uint64_t));
    }

    SHA512::Digest SHA512::calc(const void* src, size_t bytelength)
    {
        SHA512 hasher;
        hasher.update(src, bytelength);
        return hasher.finalize();
    }

} //  namespace Hash

X_NAMESPACE_END
