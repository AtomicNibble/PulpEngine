#include "EngineCommon.h"
#include "sha1.h"

X_NAMESPACE_BEGIN(core)

namespace Hash
{

#define SHA1_LITTLE_ENDIAN
#ifndef ROL32
#ifdef _MSC_VER
#define ROL32(p_val32,p_nBits) _rotl(p_val32,p_nBits)
#else
#define ROL32(p_val32,p_nBits) (((p_val32)<<(p_nBits))|((p_val32)>>(32-(p_nBits))))
#endif
#endif

#ifdef SHA1_LITTLE_ENDIAN
#define SHABLK0(i) (block_.l[i] = \
	(ROL32(block_.l[i], 24) & 0xFF00FF00) | (ROL32(block_.l[i], 8) & 0x00FF00FF))
#else
#define SHABLK0(i) (buffer.l[i])
#endif

#define SHABLK(i) (block_.l[i&15] = ROL32(block_.l[(i+13)&15] ^ \
	block_.l[(i + 8) & 15] ^ block_.l[(i + 2) & 15] ^ block_.l[i & 15], 1))

// SHA-1 rounds
#define S_R0(v,w,x,y,z,i) {z+=((w&(x^y))^y)+SHABLK0(i)+0x5A827999+ROL32(v,5);w=ROL32(w,30);}
#define S_R1(v,w,x,y,z,i) {z+=((w&(x^y))^y)+SHABLK(i)+0x5A827999+ROL32(v,5);w=ROL32(w,30);}
#define S_R2(v,w,x,y,z,i) {z+=(w^x^y)+SHABLK(i)+0x6ED9EBA1+ROL32(v,5);w=ROL32(w,30);}
#define S_R3(v,w,x,y,z,i) {z+=(((w|x)&y)|(w&x))+SHABLK(i)+0x8F1BBCDC+ROL32(v,5);w=ROL32(w,30);}
#define S_R4(v,w,x,y,z,i) {z+=(w^x^y)+SHABLK(i)+0xCA62C1D6+ROL32(v,5);w=ROL32(w,30);}

//#pragma warning(push)
//#pragma warning(disable: 4127)

	// ---------------------------------------------------------

	SHA1::SHA1()
	{
		reset();
	}

	SHA1::~SHA1()
	{

	}

	void SHA1::reset(void)
	{
		zero_object(buffer_);
		digest_[0] = 0x67452301;
		digest_[1] = 0xefcdab89;
		digest_[2] = 0x98badcfe;
		digest_[3] = 0x10325476;
		digest_[4] = 0xc3d2e1f0;
		numBytes_ = 0;
	}

	void SHA1::update(const void* buf, size_t length)
	{
		size_t index = numBytes_ % BLOCK_BYTES;
		size_t firstpart = BLOCK_BYTES - index;
		size_t i;

		const uint8_t* input = reinterpret_cast<const uint8_t*>(buf);

		if (length >= firstpart)
		{
			memcpy(&buffer_[index], input, firstpart);
			transform(buffer_);

			for (i = firstpart; i + BLOCK_BYTES <= length; i += BLOCK_BYTES) {
				transform(&input[i]);
			}

			index = 0;
		}
		else
		{
			i = 0;
		}

		numBytes_ += length;

		memcpy(&buffer_[index], &input[i], length - i);
	}

	void SHA1::update(const char* str)
	{
		update(reinterpret_cast<const void*>(str), ::strlen(str));
	}

	SHA1Digest SHA1::finalize(void)
	{
		SHA1Digest res;

		static unsigned char padding[64] = {
			0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
		};

		uint64_t size = numBytes_ << 3;

		// pad out to 56 mod 64.
		size_t index = numBytes_ % 64;
		size_t padLen = (index < 56) ? (56 - index) : (120 - index);
		update(padding, padLen);

		uint32_t numBits[2];
		numBits[0] = _byteswap_ulong(static_cast<uint32_t>(size >> 32));
		numBits[1] = _byteswap_ulong(static_cast<uint32_t>(size));
		update(numBits, 8);

		for (size_t i = 0; i < 20; ++i)
		{
			res.bytes[i] = static_cast<uint8_t>((digest_[i >> 2] >> ((3 -
				(i & 3)) * 8)) & 0xFF);
		}

		reset();
		return res;
	}

	void SHA1::transform(const uint8_t* pBuffer)
	{
		uint32_t a = digest_[0], b = digest_[1], c = digest_[2], d = digest_[3], e = digest_[4];

		memcpy(&block_, pBuffer, 64);

		// 4 rounds of 20 operations each, loop unrolled
		S_R0(a, b, c, d, e, 0); S_R0(e, a, b, c, d, 1); S_R0(d, e, a, b, c, 2); S_R0(c, d, e, a, b, 3);
		S_R0(b, c, d, e, a, 4); S_R0(a, b, c, d, e, 5); S_R0(e, a, b, c, d, 6); S_R0(d, e, a, b, c, 7);
		S_R0(c, d, e, a, b, 8); S_R0(b, c, d, e, a, 9); S_R0(a, b, c, d, e, 10); S_R0(e, a, b, c, d, 11);
		S_R0(d, e, a, b, c, 12); S_R0(c, d, e, a, b, 13); S_R0(b, c, d, e, a, 14); S_R0(a, b, c, d, e, 15);
		S_R1(e, a, b, c, d, 16); S_R1(d, e, a, b, c, 17); S_R1(c, d, e, a, b, 18); S_R1(b, c, d, e, a, 19);
		S_R2(a, b, c, d, e, 20); S_R2(e, a, b, c, d, 21); S_R2(d, e, a, b, c, 22); S_R2(c, d, e, a, b, 23);
		S_R2(b, c, d, e, a, 24); S_R2(a, b, c, d, e, 25); S_R2(e, a, b, c, d, 26); S_R2(d, e, a, b, c, 27);
		S_R2(c, d, e, a, b, 28); S_R2(b, c, d, e, a, 29); S_R2(a, b, c, d, e, 30); S_R2(e, a, b, c, d, 31);
		S_R2(d, e, a, b, c, 32); S_R2(c, d, e, a, b, 33); S_R2(b, c, d, e, a, 34); S_R2(a, b, c, d, e, 35);
		S_R2(e, a, b, c, d, 36); S_R2(d, e, a, b, c, 37); S_R2(c, d, e, a, b, 38); S_R2(b, c, d, e, a, 39);
		S_R3(a, b, c, d, e, 40); S_R3(e, a, b, c, d, 41); S_R3(d, e, a, b, c, 42); S_R3(c, d, e, a, b, 43);
		S_R3(b, c, d, e, a, 44); S_R3(a, b, c, d, e, 45); S_R3(e, a, b, c, d, 46); S_R3(d, e, a, b, c, 47);
		S_R3(c, d, e, a, b, 48); S_R3(b, c, d, e, a, 49); S_R3(a, b, c, d, e, 50); S_R3(e, a, b, c, d, 51);
		S_R3(d, e, a, b, c, 52); S_R3(c, d, e, a, b, 53); S_R3(b, c, d, e, a, 54); S_R3(a, b, c, d, e, 55);
		S_R3(e, a, b, c, d, 56); S_R3(d, e, a, b, c, 57); S_R3(c, d, e, a, b, 58); S_R3(b, c, d, e, a, 59);
		S_R4(a, b, c, d, e, 60); S_R4(e, a, b, c, d, 61); S_R4(d, e, a, b, c, 62); S_R4(c, d, e, a, b, 63);
		S_R4(b, c, d, e, a, 64); S_R4(a, b, c, d, e, 65); S_R4(e, a, b, c, d, 66); S_R4(d, e, a, b, c, 67);
		S_R4(c, d, e, a, b, 68); S_R4(b, c, d, e, a, 69); S_R4(a, b, c, d, e, 70); S_R4(e, a, b, c, d, 71);
		S_R4(d, e, a, b, c, 72); S_R4(c, d, e, a, b, 73); S_R4(b, c, d, e, a, 74); S_R4(a, b, c, d, e, 75);
		S_R4(e, a, b, c, d, 76); S_R4(d, e, a, b, c, 77); S_R4(c, d, e, a, b, 78); S_R4(b, c, d, e, a, 79);

		// Add the working vars back into state
		digest_[0] += a;
		digest_[1] += b;
		digest_[2] += c;
		digest_[3] += d;
		digest_[4] += e;

		// Wipe variables
		a = b = c = d = e = 0;
	}

} // namespace Hash

X_NAMESPACE_END