#include "stdafx.h"
#include "StringCompress.h"

namespace
{

    const tt_int32 englishCharacterFrequencies[256] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 722, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        11084, 58, 63, 1, 0, 31, 0, 317, 64, 64, 44, 0, 695, 62, 980, 266, 69, 67, 56, 7, 73, 3, 14, 2, 69,
        1, 167, 9, 1, 2, 25, 94, 0, 195, 139, 34, 96, 48, 103, 56, 125, 653, 21, 5, 23, 64, 85, 44, 34, 7, 92, 
        76, 147, 12, 14, 57, 15, 39, 15, 1, 1, 1, 2, 3, 0, 3611, 845, 1077, 1884, 5870, 841, 1057, 2501, 3212, 
        164, 531, 2019, 1330, 3056, 4037, 848, 47, 2586, 2919, 4771, 1707, 535, 1106, 152, 1243, 100, 0, 2, 0, 
        10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };


    struct CharacterEncoding
    {
        tt_uint16 offset;
        tt_uint16 bitLength;
    };

    TELEM_DISABLE_WARNING(4268)

    struct EncodedTable
    {
        CharacterEncoding chars[256];
        tt_uint8 bits[1]; // N
    };
    
    const EncodedTable table;

} // namespace

tt_size compressStringToBuf(const char* pStr, tt_uint8* pBuf, tt_size bufLength)
{
    // are you null? slap yourself!
    // this basically just need to pack it like it's hot.
    tt_size bitLen = 0;
    tt_size bufBitLeng = bufLength * 8;

    while (*pStr && bitLen < bufBitLeng)
    {
        const auto& ce = table.chars[*pStr];
        auto* pBits = &table.bits[ce.offset];

        // write one bit at time for now
        for (tt_int32 i = 0; i < ce.bitLength; i++)
        {
            auto dstBitIndex = bitLen & 7;
            auto srcBitIndex = i & 7;

            pBuf[bitLen / 8] |= (pBits[i / 8] >> srcBitIndex) << dstBitIndex;

            ++bitLen;
        }
    }

    // to bytes
    bitLen /= 8;

    return bitLen;
}