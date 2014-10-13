#include "EngineCommon.h"
#include "Fnva1Hash.h"

X_NAMESPACE_BEGIN(core)

namespace Hash
{

	/*
	 * 32 bit magic FNV-1a prime
	 */
	#define FNV_32_PRIME ((uint32_t)0x01000193)
	#define FNV1_32_INIT ((uint32_t)0x811c9dc5)
	#define FNV1_32A_INIT FNV1_32_INIT

	#define FNV_64_PRIME   ((uint64_t)0x100000001B3)
	#define FNV1_64_INIT  ((uint64_t)0x14650FB0739D0383) 
	#define FNV1_64A_INIT FNV1_64_INIT

	unsigned int Fnv1aHash(const void* key, size_t length)
	{
		unsigned int i;
		unsigned int hash = FNV1_32_INIT;
		unsigned char *s = (unsigned char *)key;

		for ( i = 0; i < length; ++i ) {
			hash ^= (uint32_t)*s++;
			hash *= FNV_32_PRIME;
		}
		
		return hash;
	}


	namespace Int64
	{
		uint64_t Fnv1aHash(const void* key, size_t length)
		{
			size_t i;
			uint64_t hash = FNV1_64_INIT;
			unsigned char *s = (unsigned char *)key;

			for ( i = 0; i < length; ++i ) {
				hash ^= (uint64_t)*s++;
				hash *= FNV_64_PRIME;
			}
		
			return hash;
		}
	}
}

X_NAMESPACE_END