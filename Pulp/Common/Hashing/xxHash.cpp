#include "EngineCommon.h"
#include "xxHash.h"

#include <Util\EndianUtil.h>


#ifdef UNALIGNED
#undef UNALIGNED
#endif // !UNALIGNED

X_NAMESPACE_BEGIN(core)

namespace Hash
{
	namespace
	{

#if defined(__ARM_FEATURE_UNALIGNED) || defined(__i386) || defined(_M_IX86) || defined(__x86_64__) || defined(_M_X64)
#  define XXH_USE_UNALIGNED_ACCESS 1
#endif

		/* XXH_ACCEPT_NULL_INPUT_POINTER :
		* If the input pointer is a null pointer, xxHash default behavior is to trigger a memory access error, since it is a bad pointer.
		* When this option is enabled, xxHash output for null input pointers will be the same as a null-length input.
		* By default, this option is disabled. To enable it, uncomment below define :
		*/
		/* #define XXH_ACCEPT_NULL_INPUT_POINTER 1 */

		/* XXH_FORCE_NATIVE_FORMAT :
		* By default, xxHash library provides endian-independant Hash values, based on little-endian convention.
		* Results are therefore identical for little-endian and big-endian CPU.
		* This comes at a performance cost for big-endian CPU, since some swapping is required to emulate little-endian format.
		* Should endian-independance be of no importance for your application, you may set the #define below to 1.
		* It will improve speed for Big-endian CPU.
		* This option has no impact on Little_Endian CPU.
		*/
#define XXH_FORCE_NATIVE_FORMAT 0


#if defined(_MSC_VER)
#  define XXH_rotl32(x,r) _rotl(x,r)
#  define XXH_rotl64(x,r) _rotl64(x,r)
#else
#  define XXH_rotl32(x,r) ((x << r) | (x >> (32 - r)))
#  define XXH_rotl64(x,r) ((x << r) | (x >> (64 - r)))
#endif


#ifndef XXH_CPU_LITTLE_ENDIAN   /* XXH_CPU_LITTLE_ENDIAN can be defined externally, for example using a compiler switch */
		static const int one = 1;
#   define XXH_CPU_LITTLE_ENDIAN   (*(const char*)(&one))
#endif

		enum class Endianes
		{
			BIG = 0,
			LITTLE = 1
		};

		enum class Alignment
		{
			ALIGNED = 1,
			UNALIGNED = 2
		};


		const uint32_t PRIME32_1 = 2654435761U;
		const uint32_t PRIME32_2 = 2246822519U;
		const uint32_t PRIME32_3 = 3266489917U;
		const uint32_t PRIME32_4 = 668265263U;
		const uint32_t PRIME32_5 = 374761393U;

		const uint64_t PRIME64_1 = 11400714785074694791ULL;
		const uint64_t PRIME64_2 = 14029467366897019727ULL;
		const uint64_t PRIME64_3 = 1609587929392839161ULL;
		const uint64_t PRIME64_4 = 9650029242287828579ULL;
		const uint64_t PRIME64_5 = 2870177450012600261ULL;

		static uint32_t XXH_read32(const void* memPtr)
		{
			uint32_t val32;
			memcpy(&val32, memPtr, 4);
			return val32;
		}

		static uint64_t XXH_read64(const void* memPtr)
		{
			uint64_t val64;
			memcpy(&val64, memPtr, 8);
			return val64;
		}


		X_INLINE uint32_t XXH_readLE32_align(const void* ptr, Endianes endian, Alignment align)
		{
			if (align == Alignment::UNALIGNED)
				return endian == Endianes::LITTLE ? XXH_read32(ptr) : Endian::swap(XXH_read32(ptr));
			else
				return endian == Endianes::LITTLE ? *(const uint32_t*)ptr : Endian::swap(*(const uint32_t*)ptr);
		}

		X_INLINE uint32_t XXH_readLE32(const void* ptr, Endianes endian)
		{
			return XXH_readLE32_align(ptr, endian, Alignment::UNALIGNED);
		}

		X_INLINE uint64_t XXH_readLE64_align(const void* ptr, Endianes endian, Alignment align)
		{
			if (align == Alignment::UNALIGNED)
				return endian == Endianes::LITTLE ? XXH_read64(ptr) : Endian::swap(XXH_read64(ptr));
			else
				return endian == Endianes::LITTLE ? *(const uint64_t*)ptr : Endian::swap(*(const uint64_t*)ptr);
		}

		X_INLINE uint64_t XXH_readLE64(const void* ptr, Endianes endian)
		{
			return XXH_readLE64_align(ptr, endian, Alignment::UNALIGNED);
		}

		X_INLINE void* XXH_memcpy(void* dest, const void* src, size_t size)
		{
			return memcpy(dest, src, size); 
		}

		struct XXH_istate32_t
		{
			uint64_t total_len;
			uint32_t seed;
			uint32_t v1;
			uint32_t v2;
			uint32_t v3;
			uint32_t v4;
			uint32_t mem32[4];   /* defined as uint32_t for alignment */
			uint32_t memsize;
		};

		struct XXH_istate64_t
		{
			uint64_t total_len;
			uint64_t seed;
			uint64_t v1;
			uint64_t v2;
			uint64_t v3;
			uint64_t v4;
			uint64_t mem64[4];   /* defined as uint64_t for alignment */
			uint32_t memsize;
		};


		X_INLINE uint32_t XXH32_endian_align(const void* input, size_t len, uint32_t seed, Endianes endian, Alignment align)
		{
			const BYTE* p = reinterpret_cast<const BYTE*>(input);
			const BYTE* bEnd = p + len;
			uint32_t h32;
#define XXH_get32bits(p) XXH_readLE32_align(p, endian, align)

#ifdef XXH_ACCEPT_NULL_INPUT_POINTER
			if (p == nullptr)
			{
				len = 0;
				bEnd = p = (const BYTE*)(size_t)16;
			}
#endif

			if (len >= 16)
			{
				const BYTE* const limit = bEnd - 16;
				uint32_t v1 = seed + PRIME32_1 + PRIME32_2;
				uint32_t v2 = seed + PRIME32_2;
				uint32_t v3 = seed + 0;
				uint32_t v4 = seed - PRIME32_1;

				do
				{
					v1 += XXH_get32bits(p) * PRIME32_2;
					v1 = XXH_rotl32(v1, 13);
					v1 *= PRIME32_1;
					p += 4;
					v2 += XXH_get32bits(p) * PRIME32_2;
					v2 = XXH_rotl32(v2, 13);
					v2 *= PRIME32_1;
					p += 4;
					v3 += XXH_get32bits(p) * PRIME32_2;
					v3 = XXH_rotl32(v3, 13);
					v3 *= PRIME32_1;
					p += 4;
					v4 += XXH_get32bits(p) * PRIME32_2;
					v4 = XXH_rotl32(v4, 13);
					v4 *= PRIME32_1;
					p += 4;
				} while (p <= limit);

				h32 = XXH_rotl32(v1, 1) + XXH_rotl32(v2, 7) + XXH_rotl32(v3, 12) + XXH_rotl32(v4, 18);
			}
			else
			{
				h32 = seed + PRIME32_5;
			}

			h32 += static_cast<uint32_t>(len);

			while (p + 4 <= bEnd)
			{
				h32 += XXH_get32bits(p) * PRIME32_3;
				h32 = XXH_rotl32(h32, 17) * PRIME32_4;
				p += 4;
			}

			while (p<bEnd)
			{
				h32 += (*p) * PRIME32_5;
				h32 = XXH_rotl32(h32, 11) * PRIME32_1;
				p++;
			}

			h32 ^= h32 >> 15;
			h32 *= PRIME32_2;
			h32 ^= h32 >> 13;
			h32 *= PRIME32_3;
			h32 ^= h32 >> 16;

			return h32;
		}


		X_INLINE bool XXH32_update_endian(XXH_istate32_t* state, const void* input, size_t len, Endianes endian)
		{
			const BYTE* p = reinterpret_cast<const BYTE*>(input);
			const BYTE* const bEnd = p + len;

#ifdef XXH_ACCEPT_NULL_INPUT_POINTER
			if (input == nullptr) {
				return false;
			}
#endif

			state->total_len += len;

			if (state->memsize + len < 16)   /* fill in tmp buffer */
			{
				XXH_memcpy((BYTE*)(state->mem32) + state->memsize, input, len);
				state->memsize += static_cast<uint32_t>(len);
				return true;
			}

			if (state->memsize)   /* some data left from previous update */
			{
				XXH_memcpy((BYTE*)(state->mem32) + state->memsize, input, 16 - state->memsize);
				{
					const uint32_t* p32 = state->mem32;
					state->v1 += XXH_readLE32(p32, endian) * PRIME32_2;
					state->v1 = XXH_rotl32(state->v1, 13);
					state->v1 *= PRIME32_1;
					p32++;
					state->v2 += XXH_readLE32(p32, endian) * PRIME32_2;
					state->v2 = XXH_rotl32(state->v2, 13);
					state->v2 *= PRIME32_1;
					p32++;
					state->v3 += XXH_readLE32(p32, endian) * PRIME32_2;
					state->v3 = XXH_rotl32(state->v3, 13);
					state->v3 *= PRIME32_1;
					p32++;
					state->v4 += XXH_readLE32(p32, endian) * PRIME32_2;
					state->v4 = XXH_rotl32(state->v4, 13);
					state->v4 *= PRIME32_1;
					p32++;
				}
				p += 16 - state->memsize;
				state->memsize = 0;
			}

			if (p <= bEnd - 16)
			{
				const BYTE* const limit = bEnd - 16;
				uint32_t v1 = state->v1;
				uint32_t v2 = state->v2;
				uint32_t v3 = state->v3;
				uint32_t v4 = state->v4;

				do
				{
					v1 += XXH_readLE32(p, endian) * PRIME32_2;
					v1 = XXH_rotl32(v1, 13);
					v1 *= PRIME32_1;
					p += 4;
					v2 += XXH_readLE32(p, endian) * PRIME32_2;
					v2 = XXH_rotl32(v2, 13);
					v2 *= PRIME32_1;
					p += 4;
					v3 += XXH_readLE32(p, endian) * PRIME32_2;
					v3 = XXH_rotl32(v3, 13);
					v3 *= PRIME32_1;
					p += 4;
					v4 += XXH_readLE32(p, endian) * PRIME32_2;
					v4 = XXH_rotl32(v4, 13);
					v4 *= PRIME32_1;
					p += 4;
				} while (p <= limit);

				state->v1 = v1;
				state->v2 = v2;
				state->v3 = v3;
				state->v4 = v4;
			}

			if (p < bEnd)
			{
				XXH_memcpy(state->mem32, p, bEnd - p);
				state->memsize = (int)(bEnd - p);
			}

			return true;
		}

		X_INLINE uint32_t XXH32_digest_endian(const XXH_istate32_t* state, Endianes endian)
		{
			const BYTE * p = reinterpret_cast<const BYTE*>(state->mem32);
			const BYTE* bEnd = reinterpret_cast<const BYTE*>(state->mem32) + state->memsize;
			uint32_t h32;

			if (state->total_len >= 16)
			{
				h32 = XXH_rotl32(state->v1, 1) + XXH_rotl32(state->v2, 7) + XXH_rotl32(state->v3, 12) + XXH_rotl32(state->v4, 18);
			}
			else
			{
				h32 = state->seed + PRIME32_5;
			}

			h32 += static_cast<uint32_t>(state->total_len);

			while (p + 4 <= bEnd)
			{
				h32 += XXH_readLE32(p, endian) * PRIME32_3;
				h32 = XXH_rotl32(h32, 17) * PRIME32_4;
				p += 4;
			}

			while (p<bEnd)
			{
				h32 += (*p) * PRIME32_5;
				h32 = XXH_rotl32(h32, 11) * PRIME32_1;
				p++;
			}

			h32 ^= h32 >> 15;
			h32 *= PRIME32_2;
			h32 ^= h32 >> 13;
			h32 *= PRIME32_3;
			h32 ^= h32 >> 16;

			return h32;
		}

		X_INLINE uint64_t XXH64_endian_align(const void* input, size_t len, uint64_t seed, Endianes endian, Alignment align)
		{
			const BYTE* p = reinterpret_cast<const BYTE*>(input);
			const BYTE* bEnd = (p + len);
			uint64_t h64;

#define XXH_get64bits(p) XXH_readLE64_align(p, endian, align)

#ifdef XXH_ACCEPT_NULL_INPUT_POINTER
			if (p == nullptr)
			{
				len = 0;
				bEnd = p = (const BYTE*)(size_t)32;
			}
#endif

			if (len >= 32)
			{
				const BYTE* const limit = bEnd - 32;
				uint64_t v1 = seed + PRIME64_1 + PRIME64_2;
				uint64_t v2 = seed + PRIME64_2;
				uint64_t v3 = seed + 0;
				uint64_t v4 = seed - PRIME64_1;

				do
				{
					v1 += XXH_get64bits(p) * PRIME64_2;
					p += 8;
					v1 = XXH_rotl64(v1, 31);
					v1 *= PRIME64_1;
					v2 += XXH_get64bits(p) * PRIME64_2;
					p += 8;
					v2 = XXH_rotl64(v2, 31);
					v2 *= PRIME64_1;
					v3 += XXH_get64bits(p) * PRIME64_2;
					p += 8;
					v3 = XXH_rotl64(v3, 31);
					v3 *= PRIME64_1;
					v4 += XXH_get64bits(p) * PRIME64_2;
					p += 8;
					v4 = XXH_rotl64(v4, 31);
					v4 *= PRIME64_1;
				} while (p <= limit);

				h64 = XXH_rotl64(v1, 1) + XXH_rotl64(v2, 7) + XXH_rotl64(v3, 12) + XXH_rotl64(v4, 18);

				v1 *= PRIME64_2;
				v1 = XXH_rotl64(v1, 31);
				v1 *= PRIME64_1;
				h64 ^= v1;
				h64 = h64 * PRIME64_1 + PRIME64_4;

				v2 *= PRIME64_2;
				v2 = XXH_rotl64(v2, 31);
				v2 *= PRIME64_1;
				h64 ^= v2;
				h64 = h64 * PRIME64_1 + PRIME64_4;

				v3 *= PRIME64_2;
				v3 = XXH_rotl64(v3, 31);
				v3 *= PRIME64_1;
				h64 ^= v3;
				h64 = h64 * PRIME64_1 + PRIME64_4;

				v4 *= PRIME64_2;
				v4 = XXH_rotl64(v4, 31);
				v4 *= PRIME64_1;
				h64 ^= v4;
				h64 = h64 * PRIME64_1 + PRIME64_4;
			}
			else
			{
				h64 = seed + PRIME64_5;
			}

			h64 += static_cast<uint64_t>(len);

			while (p + 8 <= bEnd)
			{
				uint64_t k1 = XXH_get64bits(p);
				k1 *= PRIME64_2;
				k1 = XXH_rotl64(k1, 31);
				k1 *= PRIME64_1;
				h64 ^= k1;
				h64 = XXH_rotl64(h64, 27) * PRIME64_1 + PRIME64_4;
				p += 8;
			}

			if (p + 4 <= bEnd)
			{
				h64 ^= static_cast<uint64_t>(XXH_get32bits(p)) * PRIME64_1;
				h64 = XXH_rotl64(h64, 23) * PRIME64_2 + PRIME64_3;
				p += 4;
			}

			while (p<bEnd)
			{
				h64 ^= (*p) * PRIME64_5;
				h64 = XXH_rotl64(h64, 11) * PRIME64_1;
				p++;
			}

			h64 ^= h64 >> 33;
			h64 *= PRIME64_2;
			h64 ^= h64 >> 29;
			h64 *= PRIME64_3;
			h64 ^= h64 >> 32;

			return h64;
		}

		X_INLINE bool XXH64_update_endian(XXH_istate64_t* state, const void* input, size_t len, Endianes endian)
		{
			const BYTE * p = reinterpret_cast<const BYTE*>(input);
			const BYTE* const bEnd = reinterpret_cast<const BYTE*>(p + len);

#ifdef XXH_ACCEPT_NULL_INPUT_POINTER
			if (input == nullptr) {
				return false;
			}
#endif

			state->total_len += len;

			if (state->memsize + len < 32)   /* fill in tmp buffer */
			{
				XXH_memcpy(((BYTE*)state->mem64) + state->memsize, input, len);
				state->memsize += static_cast<uint32_t>(len);
				return true;
			}

			if (state->memsize)   /* some data left from previous update */
			{
				XXH_memcpy(((BYTE*)state->mem64) + state->memsize, input, 32 - state->memsize);
				{
					const uint64_t* p64 = state->mem64;
					state->v1 += XXH_readLE64(p64, endian) * PRIME64_2;
					state->v1 = XXH_rotl64(state->v1, 31);
					state->v1 *= PRIME64_1;
					p64++;
					state->v2 += XXH_readLE64(p64, endian) * PRIME64_2;
					state->v2 = XXH_rotl64(state->v2, 31);
					state->v2 *= PRIME64_1;
					p64++;
					state->v3 += XXH_readLE64(p64, endian) * PRIME64_2;
					state->v3 = XXH_rotl64(state->v3, 31);
					state->v3 *= PRIME64_1;
					p64++;
					state->v4 += XXH_readLE64(p64, endian) * PRIME64_2;
					state->v4 = XXH_rotl64(state->v4, 31);
					state->v4 *= PRIME64_1;
					p64++;
				}
				p += 32 - state->memsize;
				state->memsize = 0;
			}

			if (p + 32 <= bEnd)
			{
				const BYTE* const limit = bEnd - 32;
				uint64_t v1 = state->v1;
				uint64_t v2 = state->v2;
				uint64_t v3 = state->v3;
				uint64_t v4 = state->v4;

				do
				{
					v1 += XXH_readLE64(p, endian) * PRIME64_2;
					v1 = XXH_rotl64(v1, 31);
					v1 *= PRIME64_1;
					p += 8;
					v2 += XXH_readLE64(p, endian) * PRIME64_2;
					v2 = XXH_rotl64(v2, 31);
					v2 *= PRIME64_1;
					p += 8;
					v3 += XXH_readLE64(p, endian) * PRIME64_2;
					v3 = XXH_rotl64(v3, 31);
					v3 *= PRIME64_1;
					p += 8;
					v4 += XXH_readLE64(p, endian) * PRIME64_2;
					v4 = XXH_rotl64(v4, 31);
					v4 *= PRIME64_1;
					p += 8;
				} while (p <= limit);

				state->v1 = v1;
				state->v2 = v2;
				state->v3 = v3;
				state->v4 = v4;
			}

			if (p < bEnd)
			{
				XXH_memcpy(state->mem64, p, bEnd - p);
				state->memsize = (int)(bEnd - p);
			}

			return true;
		}

		X_INLINE uint64_t XXH64_digest_endian(const XXH_istate64_t* state, Endianes endian)
		{
			const BYTE * p = reinterpret_cast<const BYTE*>(state->mem64);
			const BYTE* bEnd = reinterpret_cast<const BYTE*>(state->mem64) + state->memsize;
			uint64_t h64;

			if (state->total_len >= 32)
			{
				uint64_t v1 = state->v1;
				uint64_t v2 = state->v2;
				uint64_t v3 = state->v3;
				uint64_t v4 = state->v4;

				h64 = XXH_rotl64(v1, 1) + XXH_rotl64(v2, 7) + XXH_rotl64(v3, 12) + XXH_rotl64(v4, 18);

				v1 *= PRIME64_2;
				v1 = XXH_rotl64(v1, 31);
				v1 *= PRIME64_1;
				h64 ^= v1;
				h64 = h64*PRIME64_1 + PRIME64_4;

				v2 *= PRIME64_2;
				v2 = XXH_rotl64(v2, 31);
				v2 *= PRIME64_1;
				h64 ^= v2;
				h64 = h64*PRIME64_1 + PRIME64_4;

				v3 *= PRIME64_2;
				v3 = XXH_rotl64(v3, 31);
				v3 *= PRIME64_1;
				h64 ^= v3;
				h64 = h64*PRIME64_1 + PRIME64_4;

				v4 *= PRIME64_2;
				v4 = XXH_rotl64(v4, 31);
				v4 *= PRIME64_1;
				h64 ^= v4;
				h64 = h64*PRIME64_1 + PRIME64_4;
			}
			else
			{
				h64 = state->seed + PRIME64_5;
			}

			h64 += static_cast<uint64_t>(state->total_len);

			while (p + 8 <= bEnd)
			{
				uint64_t k1 = XXH_readLE64(p, endian);
				k1 *= PRIME64_2;
				k1 = XXH_rotl64(k1, 31);
				k1 *= PRIME64_1;
				h64 ^= k1;
				h64 = XXH_rotl64(h64, 27) * PRIME64_1 + PRIME64_4;
				p += 8;
			}

			if (p + 4 <= bEnd)
			{
				h64 ^= static_cast<uint64_t>((XXH_readLE32(p, endian)) * PRIME64_1);
				h64 = XXH_rotl64(h64, 23) * PRIME64_2 + PRIME64_3;
				p += 4;
			}

			while (p<bEnd)
			{
				h64 ^= (*p) * PRIME64_5;
				h64 = XXH_rotl64(h64, 11) * PRIME64_1;
				p++;
			}

			h64 ^= h64 >> 33;
			h64 *= PRIME64_2;
			h64 ^= h64 >> 29;
			h64 *= PRIME64_3;
			h64 ^= h64 >> 32;

			return h64;
		}

	} // namespace


	xxHash32::xxHash32()
	{
		reset(0);
	}


	void xxHash32::reset(uint32_t seed)
	{
		XXH_istate32_t* state = reinterpret_cast<XXH_istate32_t*>(&state_);
		state->seed = seed;
		state->v1 = seed + PRIME32_1 + PRIME32_2;
		state->v2 = seed + PRIME32_2;
		state->v3 = seed + 0;
		state->v4 = seed - PRIME32_1;
		state->total_len = 0;
		state->memsize = 0;
	}

	bool xxHash32::update(const void* pBuf, size_t length)
	{
		Endianes endian_detected = (Endianes)XXH_CPU_LITTLE_ENDIAN;

		if ((endian_detected == Endianes::LITTLE) || XXH_FORCE_NATIVE_FORMAT)
			return XXH32_update_endian(reinterpret_cast<XXH_istate32_t*>(&state_), pBuf, length, Endianes::LITTLE);
		else
			return XXH32_update_endian(reinterpret_cast<XXH_istate32_t*>(&state_), pBuf, length, Endianes::BIG);
	}

	uint32_t xxHash32::finalize(void)
	{
		Endianes endian_detected = (Endianes)XXH_CPU_LITTLE_ENDIAN;

		if ((endian_detected == Endianes::LITTLE) || XXH_FORCE_NATIVE_FORMAT)
			return XXH32_digest_endian(reinterpret_cast<XXH_istate32_t*>(&state_), Endianes::LITTLE);
		else
			return XXH32_digest_endian(reinterpret_cast<XXH_istate32_t*>(&state_), Endianes::BIG);
	}

	uint32_t xxHash32::getHash(const void* pInput, size_t length, uint32_t seed)
	{
		Endianes endian_detected = (Endianes)XXH_CPU_LITTLE_ENDIAN;

		if ((((size_t)pInput) & 3) == 0)   /* Input is 4-bytes aligned, leverage the speed benefit */
		{
			if ((endian_detected == Endianes::LITTLE) || XXH_FORCE_NATIVE_FORMAT)
				return XXH32_endian_align(pInput, length, seed, Endianes::LITTLE, Alignment::ALIGNED);
			else
				return XXH32_endian_align(pInput, length, seed, Endianes::BIG, Alignment::ALIGNED);
		}

		if ((endian_detected == Endianes::LITTLE) || XXH_FORCE_NATIVE_FORMAT)
			return XXH32_endian_align(pInput, length, seed, Endianes::LITTLE, Alignment::UNALIGNED);
		else
			return XXH32_endian_align(pInput, length, seed, Endianes::BIG, Alignment::UNALIGNED);

	}

	// ---------------------------------------


	xxHash64::xxHash64()
	{
		reset(0);
	}


	void xxHash64::reset(uint64_t seed)
	{
		XXH_istate64_t* state = reinterpret_cast<XXH_istate64_t*>(&state_);
		state->seed = seed;
		state->v1 = seed + PRIME64_1 + PRIME64_2;
		state->v2 = seed + PRIME64_2;
		state->v3 = seed + 0;
		state->v4 = seed - PRIME64_1;
		state->total_len = 0;
		state->memsize = 0;
	}

	bool xxHash64::update(const void* pBuf, size_t length)
	{
		Endianes endian_detected = (Endianes)XXH_CPU_LITTLE_ENDIAN;

		if ((endian_detected == Endianes::LITTLE) || XXH_FORCE_NATIVE_FORMAT)
			return XXH64_update_endian(reinterpret_cast<XXH_istate64_t*>(&state_), pBuf, length, Endianes::LITTLE);
		else
			return XXH64_update_endian(reinterpret_cast<XXH_istate64_t*>(&state_), pBuf, length, Endianes::BIG);
	}

	uint64_t xxHash64::finalize(void)
	{
		Endianes endian_detected = (Endianes)XXH_CPU_LITTLE_ENDIAN;

		if ((endian_detected == Endianes::LITTLE) || XXH_FORCE_NATIVE_FORMAT)
			return XXH64_digest_endian(reinterpret_cast<XXH_istate64_t*>(&state_), Endianes::LITTLE);
		else
			return XXH64_digest_endian(reinterpret_cast<XXH_istate64_t*>(&state_), Endianes::BIG);
	}

	uint64_t xxHash64::getHash(const void* pInput, size_t length, uint64_t seed)
	{
		Endianes endian_detected = (Endianes)XXH_CPU_LITTLE_ENDIAN;

		if ((((size_t)pInput) & 7) == 0)   /* Input is aligned, let's leverage the speed advantage */
		{
			if ((endian_detected == Endianes::LITTLE) || XXH_FORCE_NATIVE_FORMAT)
				return XXH64_endian_align(pInput, length, seed, Endianes::LITTLE, Alignment::ALIGNED);
			else
				return XXH64_endian_align(pInput, length, seed, Endianes::BIG, Alignment::ALIGNED);
		}

		if ((endian_detected == Endianes::LITTLE) || XXH_FORCE_NATIVE_FORMAT)
			return XXH64_endian_align(pInput, length, seed, Endianes::LITTLE, Alignment::UNALIGNED);
		else
			return XXH64_endian_align(pInput, length, seed, Endianes::BIG, Alignment::UNALIGNED);

	}

} // namespace Hash

X_NAMESPACE_END