#include "EngineCommon.h"
#include "sha1.h"

X_NAMESPACE_BEGIN(core)

namespace Hash
{

	namespace // local
	{
		// Rotate an integer value to left.
		inline const unsigned int rol(const unsigned int value,
			const unsigned int steps)
		{
			return ((value << steps) | (value >> (32 - steps)));
		}

		// Sets the first 16 integers in the buffert to zero.
		// Used for clearing the W buffert.
		inline void clearWBuffert(unsigned int* buffert)
		{
			for (int pos = 16; --pos >= 0;)
			{
				buffert[pos] = 0;
			}
		}

		static void SHA1_HashBlock(Sha1Hash& hash)
		{
			int t;
			UINT32 w[80];

			// Prepare Message Schedule, {W sub t}.
			//
			int j;
			for (t = 0, j = 0; t <= 15; t++, j += 4)
			{
				w[t] = (hash.block[j] << 24)
					| (hash.block[j + 1] << 16)
					| (hash.block[j + 2] << 8)
					| (hash.block[j + 3]);
			}

			UINT32 a = hash.H[0];
			UINT32 b = hash.H[1];
			UINT32 c = hash.H[2];
			UINT32 d = hash.H[3];
			UINT32 e = hash.H[4];

			int round = 0;

			#define sha1macro(func,val) \
			{ \
			const unsigned int t = rol(a, 5) + (func)+e + val + w[round]; \
			e = d; \
			d = c; \
			c = rol(b, 30); \
			b = a; \
			a = t; \
			}

			while (round < 16)
			{
				sha1macro((b & c) | (~b & d), 0x5a827999)
					++round;
			}
			while (round < 20)
			{
				w[round] = rol((w[round - 3] ^ w[round - 8] ^ w[round - 14] ^ w[round - 16]), 1);
				sha1macro((b & c) | (~b & d), 0x5a827999)
					++round;
			}
			while (round < 40)
			{
				w[round] = rol((w[round - 3] ^ w[round - 8] ^ w[round - 14] ^ w[round - 16]), 1);
				sha1macro(b ^ c ^ d, 0x6ed9eba1)
					++round;
			}
			while (round < 60)
			{
				w[round] = rol((w[round - 3] ^ w[round - 8] ^ w[round - 14] ^ w[round - 16]), 1);
				sha1macro((b & c) | (b & d) | (c & d), 0x8f1bbcdc)
					++round;
			}
			while (round < 80)
			{
				w[round] = rol((w[round - 3] ^ w[round - 8] ^ w[round - 14] ^ w[round - 16]), 1);
				sha1macro(b ^ c ^ d, 0xca62c1d6)
					++round;
			}

			#undef sha1macro
			hash.H[0] += a;
			hash.H[1] += b;
			hash.H[2] += c;
			hash.H[3] += d;
			hash.H[4] += e;
		}
	} // namespace

	void Sha1Init(Sha1Hash& hash)
	{
		hash.H[0] = 0x67452301;
		hash.H[1] = 0xEFCDAB89;
		hash.H[2] = 0x98BADCFE;
		hash.H[3] = 0x10325476;
		hash.H[4] = 0xC3D2E1F0;
		hash.nTotal = 0;
		hash.nblock = 0;
	}

	void Sha1Update(Sha1Hash& hash, const void *data, size_t n)
	{
		const char* buf = (const char*)data;
		while (n)
		{
			size_t m = sizeof(hash.block) - hash.nblock;
			if (n < m)
			{
				m = n;
			}
			memcpy(hash.block + hash.nblock, buf, m);
			buf += m;
			n -= m;
			hash.nblock += m;
			hash.nTotal += m;

			if (hash.nblock == sizeof(hash.block))
			{
				SHA1_HashBlock(hash);
				hash.nblock = 0;
			}
		}
	}

	void Sha1Final(Sha1Hash& hash)
	{
		hash.block[hash.nblock++] = 0x80;
		if (sizeof(hash.block) - sizeof(UINT64) <= hash.nblock)
		{
			memset(hash.block + hash.nblock, 0, sizeof(hash.block) - hash.nblock);
			SHA1_HashBlock(hash);
			memset(hash.block, 0, sizeof(hash.block) - sizeof(UINT64));
		}
		else
		{
			memset(hash.block + hash.nblock, 0, sizeof(hash.block) - hash.nblock - sizeof(UINT64));
		}
		hash.nTotal *= 8;

		hash.block[sizeof(hash.block) - 8] = (UINT8)((hash.nTotal >> 56) & 0xFF);
		hash.block[sizeof(hash.block) - 7] = (UINT8)((hash.nTotal >> 48) & 0xFF);
		hash.block[sizeof(hash.block) - 6] = (UINT8)((hash.nTotal >> 40) & 0xFF);
		hash.block[sizeof(hash.block) - 5] = (UINT8)((hash.nTotal >> 32) & 0xFF);
		hash.block[sizeof(hash.block) - 4] = (UINT8)((hash.nTotal >> 24) & 0xFF);
		hash.block[sizeof(hash.block) - 3] = (UINT8)((hash.nTotal >> 16) & 0xFF);
		hash.block[sizeof(hash.block) - 2] = (UINT8)((hash.nTotal >> 8) & 0xFF);
		hash.block[sizeof(hash.block) - 1] = (UINT8)((hash.nTotal) & 0xFF);
		SHA1_HashBlock(hash);
	}


	void Sha1ToString(const Sha1Hash& hash, Sha1Hash::TextValue& hexstring)
	{
		const char hexDigits[] = { "0123456789abcdef" };

		uint32_t flip[5];
		memcpy(flip, (BYTE*)hash.H, sizeof(flip));

		for (int i = 0; i < 5; i++)
		{
			flip[i] = _byteswap_ulong(flip[i]);
		}

		BYTE* pHash = (BYTE*)flip;

		for (int hashByte = 20; --hashByte >= 0;)
		{
			hexstring[hashByte << 1] = hexDigits[(pHash[hashByte] >> 4) & 0xf];
			hexstring[(hashByte << 1) + 1] = hexDigits[pHash[hashByte] & 0xf];
		}
		hexstring[40] = 0;
	}

}

X_NAMESPACE_END