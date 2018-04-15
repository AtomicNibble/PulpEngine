#include "EngineCommon.h"
#include "MurmurHash.h"

X_NAMESPACE_BEGIN(core)

namespace Hash
{
    MurmurHash2Val MurmurHash2(const void* pKey, size_t numBytes, uint32_t seed)
    {
        uint32_t len = safe_static_cast<uint32_t>(numBytes);

        // 'm' and 'r' are mixing constants generated off line.
        // They're not really 'magic', they just happen to work well.
        const uint32_t m = 0x5bd1e995;
        const int32_t r = 24;

        // Initialize the hash to a 'random' value
        uint32_t h = seed ^ len;

        // Mix 4 bytes at a time into the hash
        const uint8_t* data = static_cast<const uint8_t*>(pKey);

        while (len >= 4) {
            uint32_t k = *(uint32_t*)data;

            k *= m;
            k ^= k >> r;
            k *= m;

            h *= m;
            h ^= k;

            data += 4;
            len -= 4;
        }

        // Handle the last few bytes of the input array

        switch (len) {
            case 3:
                h ^= data[2] << 16;
            case 2:
                h ^= data[1] << 8;
            case 1:
                h ^= data[0];
                h *= m;
        };

        // Do a few final mixes of the hash to ensure the last few
        // bytes are well-incorporated.
        h ^= h >> 13;
        h *= m;
        h ^= h >> 15;

        return h;
    }

} // namespace Hash

X_NAMESPACE_END