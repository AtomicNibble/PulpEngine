#include "EngineCommon.h"
#include "Adler32.h"

X_NAMESPACE_BEGIN(core)

namespace Hash
{
#define BASE 65521
#define NMAX 5552

#define DO1(buf, i)        \
    {                      \
        adler += (buf)[i]; \
        sum2 += adler;     \
    }
#define DO2(buf, i) \
    DO1(buf, i);    \
    DO1(buf, i + 1);
#define DO4(buf, i) \
    DO2(buf, i);    \
    DO2(buf, i + 2);
#define DO8(buf, i) \
    DO4(buf, i);    \
    DO4(buf, i + 4);
#define DO16(buf) \
    DO8(buf, 0);  \
    DO8(buf, 8);

#define MOD(a) a %= BASE
#define MOD28(a) a %= BASE
#define MOD63(a) a %= BASE

    Adler32Val Adler32(const char* str)
    {
        size_t length = strUtil::strlen(str);
        return Adler32(span<const char>( str, length ));
    }

    Adler32Val Adler32(span<const char> buf)
    {
        return Adler32(span<const uint8_t>(reinterpret_cast<const uint8_t*>(buf.data()), buf.length()));
    }

    Adler32Val Adler32(span<const uint8_t> buf)
    {
        Adler32Val val = 1u;
        return Adler32(val, buf);
    }

    Adler32Val Adler32(Adler32Val& adler, span<const uint8_t> buf)
    {
        const uint8_t* pBuf = buf.data();
        auto len = buf.length();

        Adler32Val sum2 = (adler >> 16) & 0xffff;
        adler &= 0xffff;

        if (buf.empty()) {
            adler += 1L;
        }
        else if (len == 1) {
            adler += pBuf[0];
            if (adler >= BASE) {
                adler -= BASE;
            }
            sum2 += adler;
            if (sum2 >= BASE) {
                sum2 -= BASE;
            }
            return adler | (sum2 << 16);
        }

        if (len < 16) {
            while (len--) {
                adler += *pBuf++;
                sum2 += adler;
            }
            if (adler >= BASE) {
                adler -= BASE;
            }
            MOD28(sum2);
            return adler | (sum2 << 16);
        }

        Adler32Val n;

        while (len >= NMAX) {
            len -= NMAX;
            n = NMAX / 16;
            do {
                DO16(pBuf);
                pBuf += 16;
            } while (--n);
            MOD(adler);
            MOD(sum2);
        }

        if (len) {
            while (len >= 16) {
                len -= 16;
                DO16(pBuf);
                pBuf += 16;
            }
            while (len--) {
                adler += *pBuf++;
                sum2 += adler;
            }
            MOD(adler);
            MOD(sum2);
        }

        return adler | (sum2 << 16);
    }

} // namespace Hash

X_NAMESPACE_END