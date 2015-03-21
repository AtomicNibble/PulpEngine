#include "EngineCommon.h"
#include "Adler32.h"

X_NAMESPACE_BEGIN(core)

namespace Hash
{

#define BASE 65521UL    // largest prime smaller than 65536 
#define NMAX 5552		// NMAX is the largest n such that 255n(n+1)/2 + (n+1)(BASE-1) <= 2^32-1 

#define DO1(buf,i)  {s1 += buf[i]; s2 += s1;}
#define DO2(buf,i)  DO1(buf,i); DO1(buf,i+1);
#define DO4(buf,i)  DO2(buf,i); DO2(buf,i+2);
#define DO8(buf,i)  DO4(buf,i); DO4(buf,i+4);
#define DO16(buf)   DO8(buf,0); DO8(buf,8);

#ifdef NO_DIVIDE
#  define MOD(a) \
		do {
		\
		if (a >= (BASE << 16)) a -= (BASE << 16); \
		if (a >= (BASE << 15)) a -= (BASE << 15); \
		if (a >= (BASE << 14)) a -= (BASE << 14); \
		if (a >= (BASE << 13)) a -= (BASE << 13); \
		if (a >= (BASE << 12)) a -= (BASE << 12); \
		if (a >= (BASE << 11)) a -= (BASE << 11); \
		if (a >= (BASE << 10)) a -= (BASE << 10); \
		if (a >= (BASE << 9)) a -= (BASE << 9); \
		if (a >= (BASE << 8)) a -= (BASE << 8); \
		if (a >= (BASE << 7)) a -= (BASE << 7); \
		if (a >= (BASE << 6)) a -= (BASE << 6); \
		if (a >= (BASE << 5)) a -= (BASE << 5); \
		if (a >= (BASE << 4)) a -= (BASE << 4); \
		if (a >= (BASE << 3)) a -= (BASE << 3); \
		if (a >= (BASE << 2)) a -= (BASE << 2); \
		if (a >= (BASE << 1)) a -= (BASE << 1); \
		if (a >= BASE) a -= BASE; \
		} while (0)
#else
#  define MOD(a) a %= BASE
#endif


	Adler32Val Adler32(const char* str)
	{
		size_t length = strlen(str);
		return Adler32(str,length);
	}

	Adler32Val Adler32(const char* str, size_t length)
	{
		return Adler32(reinterpret_cast<const void*>(str), length);
	}

	Adler32Val Adler32(const void* buf, size_t length)
	{
		Adler32Val val = 0;
		return Adler32(val, buf, length);
	}

	Adler32Val Adler32(Adler32Val& adler, const void* bufVoid, size_t length)
	{
		uint32_t s1 = adler & 0xffff;
		uint32_t s2 = (adler >> 16) & 0xffff;
		const uint8_t* buf = reinterpret_cast<const uint8_t*>(bufVoid);
		int k;

		if (length == 0)
			return 1L;

		while (length > 0)
		{
			k = length < NMAX ? (int)length : NMAX;
			length -= k;
			while (k >= 16) {
				DO16(buf);
				buf += 16;
				k -= 16;
			}
			if (k != 0) do {
				s1 += *buf++;
				s2 += s1;
			} while (--k);
			MOD(s1);
			MOD(s2);
		}

		adler = (s2 << 16) | s1;
		return adler;
	}


} // namespace Hash

X_NAMESPACE_END