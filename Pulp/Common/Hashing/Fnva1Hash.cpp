#include "EngineCommon.h"
#include "Fnva1Hash.h"

X_NAMESPACE_BEGIN(core)

namespace Hash
{
    /*
	 * 32 bit magic FNV-1a prime
	 */
    static const uint32_t FNV_32_PRIME = 16777619u;
    static const uint32_t FNV1_32_INIT = 2166136261u;

    Fnv1aVal Fnv1aHash(const void* key, size_t length)
    {
        uint32_t i;
        uint32_t hash = FNV1_32_INIT;
        unsigned char* s = (unsigned char*)key;

        for (i = 0; i < length; ++i) {
            hash ^= (uint32_t)*s++;
            hash *= FNV_32_PRIME;
        }

        return hash;
    }

    Fnv1Val Fnv1Hash(const void* key, size_t length)
    {
        uint32_t i;
        uint32_t hash = FNV1_32_INIT;
        unsigned char* s = (unsigned char*)key;

        for (i = 0; i < length; ++i) {
            hash *= FNV_32_PRIME;
            hash ^= (uint32_t)*s++;
        }

        return hash;
    }

    namespace Int64
    {
        static const uint64_t FNV_64_PRIME = 0x100000001B3;
        static const uint64_t FNV1_64_INIT = 0x14650FB0739D0383;

        Fnv1aVal Fnv1aHash(const void* key, size_t length)
        {
            size_t i;
            uint64_t hash = FNV1_64_INIT;
            unsigned char* s = (unsigned char*)key;

            for (i = 0; i < length; ++i) {
                hash ^= (uint64_t)*s++;
                hash *= FNV_64_PRIME;
            }

            return hash;
        }

        Fnv1Val Fnv1Hash(const void* key, size_t length)
        {
            size_t i;
            uint64_t hash = FNV1_64_INIT;
            unsigned char* s = (unsigned char*)key;

            for (i = 0; i < length; ++i) {
                hash *= FNV_64_PRIME;
                hash ^= (uint64_t)*s++;
            }

            return hash;
        }
    } // namespace Int64
} // namespace Hash

X_NAMESPACE_END