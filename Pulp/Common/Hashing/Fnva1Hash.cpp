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
        return Fnv1aHash(key, length, FNV1_32_INIT);
    }

    X_INLINE Fnv1aVal Fnv1aHash(const void* key, size_t length, Fnv1aVal seed)
    {
        uint32_t i;
        Fnv1aVal hash = seed;
        auto* s = reinterpret_cast<const uint8_t*>(key);

        for (i = 0; i < length; ++i) {
            hash ^= (uint32_t)*s++;
            hash *= FNV_32_PRIME;
        }

        return hash;
    }

    Fnv1Val Fnv1Hash(const void* key, size_t length)
    {
        return Fnv1Hash(key, length, FNV1_32_INIT);
    }

    X_INLINE Fnv1Val Fnv1Hash(const void* key, size_t length, Fnv1Val seed)
    {
        uint32_t i;
        Fnv1Val hash = seed;
        auto* s = reinterpret_cast<const uint8_t*>(key);

        for (i = 0; i < length; ++i) {
            hash *= FNV_32_PRIME;
            hash ^= (uint32_t)*s++;
        }

        return hash;
    }

    static const uint64_t FNV_64_PRIME = 0x100000001B3;
    static const uint64_t FNV1_64_INIT = 0x14650FB0739D0383;

    Fnv1a64Val Fnv1a64Hash(const void* key, size_t length)
    {
        return Fnv1a64Hash(key, length, FNV1_64_INIT);
    }

    Fnv1a64Val Fnv1a64Hash(const void* key, size_t length, Fnv1a64Val seed)
    {
        size_t i;
        Fnv1a64Val hash = seed;
        auto* s = reinterpret_cast<const uint8_t*>(key);

        for (i = 0; i < length; ++i) {
            hash ^= (uint64_t)*s++;
            hash *= FNV_64_PRIME;
        }

        return hash;
    }

    Fnv164Val Fnv164Hash(const void* key, size_t length)
    {
        return Fnv164Hash(key, length, FNV1_64_INIT);
    }

    Fnv164Val Fnv164Hash(const void* key, size_t length, Fnv164Val seed)
    {
        size_t i;
        Fnv164Val hash = seed;
        auto* s = reinterpret_cast<const uint8_t*>(key);

        for (i = 0; i < length; ++i) {
            hash *= FNV_64_PRIME;
            hash ^= (uint64_t)*s++;
        }

        return hash;
    }
} // namespace Hash

X_NAMESPACE_END
