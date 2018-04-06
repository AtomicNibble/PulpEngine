#include "EngineCommon.h"
#include "Digest.h"

X_NAMESPACE_BEGIN(core)

namespace Hash
{
    const char* DigestBase::ToString(char* pBuf, size_t bufSize, const uint8_t* pDigest, size_t numBytes)
    {
        X_ASSERT(bufSize >= ((numBytes * 2) + 1), "Buffer too small")(bufSize, numBytes);

        const char hexDigits[] = {"0123456789abcdef"};

        for (int32_t hashByte = static_cast<int32_t>(numBytes); --hashByte >= 0;) {
            pBuf[hashByte << 1] = hexDigits[(pDigest[hashByte] >> 4) & 0xf];
            pBuf[(hashByte << 1) + 1] = hexDigits[pDigest[hashByte] & 0xf];
        }

        pBuf[numBytes * 2] = 0;
        return pBuf;
    }

} // namespace Hash

X_NAMESPACE_END