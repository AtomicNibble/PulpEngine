#include "EngineCommon.h"
#include "Base64.h"

#include "Containers\Array.h"

X_NAMESPACE_BEGIN(core)

namespace
{
    static const uint8_t base64_chars[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                            "abcdefghijklmnopqrstuvwxyz"
                                            "0123456789+/";

    size_t GetCharIdx(uint8_t c)
    {
        for (size_t i = 0; i < sizeof(base64_chars); i++) {
            if (base64_chars[i] == c) {
                return i;
            }
        }
        return (size_t)-1;
    }
} // namespace

uint8 Base64::EncodingAlphabet[64] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',
    'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y',
    'Z', 'a', 'b', 'c', 'd', 'e',
    'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r',
    's', 't', 'u', 'v', 'w', 'x',
    'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'};

uint8 Base64::DecodingAlphabet[256] = {0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0x3E, 0xFF, 0xFF, 0xFF, 0x3F, 0x34, 0x35,
    0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D,
    0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00,
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
    0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
    0x19, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x1A,
    0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22,
    0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A,
    0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32,
    0x33, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

core::string Base64::Encode(const core::string& str)
{
    core::string encoded;
    core::string::size_type length = str.length();
    core::string::size_type ExpectedLength = (length + 2) / 3 * 4;

    encoded.reserve(ExpectedLength);

    char EncodedBytes[5];
    EncodedBytes[4] = 0;

    const char* Source = str.data();

    while (length >= 3) {
        uint8 A = *Source++;
        uint8 B = *Source++;
        uint8 C = *Source++;
        length -= 3;

        uint32 ByteTriplet = A << 16 | B << 8 | C;
        EncodedBytes[3] = EncodingAlphabet[ByteTriplet & 0x3F];
        ByteTriplet >>= 6;
        EncodedBytes[2] = EncodingAlphabet[ByteTriplet & 0x3F];
        ByteTriplet >>= 6;
        EncodedBytes[1] = EncodingAlphabet[ByteTriplet & 0x3F];
        ByteTriplet >>= 6;
        EncodedBytes[0] = EncodingAlphabet[ByteTriplet & 0x3F];

        encoded += EncodedBytes;
    }

    if (length > 0) {
        uint8 A = *Source++;
        uint8 B = 0;
        uint8 C = 0;
        // Grab the second character if it is a 2 uint8 finish
        if (length == 2) {
            B = *Source;
        }
        uint32 ByteTriplet = A << 16 | B << 8 | C;
        // Pad with = to make a 4 uint8 chunk
        EncodedBytes[3] = '=';
        ByteTriplet >>= 6;
        // If there's only one 1 uint8 left in the source, then you need 2 pad chars
        if (length == 1) {
            EncodedBytes[2] = '=';
        }
        else {
            EncodedBytes[2] = EncodingAlphabet[ByteTriplet & 0x3F];
        }
        // Now encode the remaining bits the same way
        ByteTriplet >>= 6;
        EncodedBytes[1] = EncodingAlphabet[ByteTriplet & 0x3F];
        ByteTriplet >>= 6;
        EncodedBytes[0] = EncodingAlphabet[ByteTriplet & 0x3F];

        encoded += EncodedBytes;
    }

    return encoded;
}

core::string Base64::Decode(const core::string& str)
{
    core::Array<char> TempBuf(gEnv->pArena);
    core::string::size_type length = str.length();
    core::string::size_type ExpectedLength = length / 4 * 3;

    uint32_t PadCount = 0;
    uint8 DecodedValues[4];

    TempBuf.resize(ExpectedLength);

    const char* Source = str.data();
    char* Dest = TempBuf.begin();

    while (length) {
        // Decode the next 4 BYTEs
        for (int32 Index = 0; Index < 4; Index++) {
            // Tell the caller if there were any pad bytes
            if (*Source == '=') {
                PadCount++;
            }
            DecodedValues[Index] = DecodingAlphabet[(int32)(*Source++)];
            // Abort on values that we don't understand
            if (DecodedValues[Index] == 0xFF) {
                X_ERROR("Base64", "Unable to decode a value in source string");
                return core::string();
            }
        }

        length -= 4;
        // Rebuild the original 3 bytes from the 4 chunks of 6 bits
        uint32 OriginalTriplet = DecodedValues[0];
        OriginalTriplet <<= 6;
        OriginalTriplet |= DecodedValues[1];
        OriginalTriplet <<= 6;
        OriginalTriplet |= DecodedValues[2];
        OriginalTriplet <<= 6;
        OriginalTriplet |= DecodedValues[3];
        // Now we can tear the uint32 into bytes
        Dest[2] = OriginalTriplet & 0xFF;
        OriginalTriplet >>= 8;
        Dest[1] = OriginalTriplet & 0xFF;
        OriginalTriplet >>= 8;
        Dest[0] = OriginalTriplet & 0xFF;
        Dest += 3;
    }

    return core::string(TempBuf.begin(), TempBuf.end());
}

core::string Base64::EncodeBytes(const uint8_t* pData, size_t len)
{
    uint8_t char_array_3[3];
    uint8_t char_array_4[4];

    core::string ret;
    core::string::size_type ExpectedLength = (len + 2) / 3 * 4;

    ret.reserve(ExpectedLength);

    size_t i = 0;
    size_t j = 0;

    while (len--) {
        char_array_3[i++] = *(pData++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (i = 0; (i < 4); i++) {
                ret += base64_chars[char_array_4[i]];
            }

            i = 0;
        }
    }

    if (i) {
        for (j = i; j < 3; j++) {
            char_array_3[j] = '\0';
        }

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for (j = 0; (j < i + 1); j++) {
            ret += base64_chars[char_array_4[j]];
        }

        while ((i++ < 3)) {
            ret += '=';
        }
    }

    return ret;
}

void Base64::DecodeBytes(core::string& str, core::Array<uint8_t>& out)
{
    size_t len = str.size();
    size_t i = 0;
    size_t j = 0;
    size_t in_ = 0;
    size_t ExpectedLength = (len / 4) * 3;
    uint8_t char_array_4[4], char_array_3[3];

    out.clear();
    out.reserve(ExpectedLength);

    while (len-- && (str[in_] != '=') && is_base64(str[in_])) {
        char_array_4[i++] = str[in_];
        in_++;
        if (i == 4) {
            for (i = 0; i < 4; i++) {
                char_array_4[i] = GetCharIdx(char_array_4[i]) & 0xFF;
            }

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for (i = 0; (i < 3); i++) {
                out.append(char_array_3[i]);
            }

            i = 0;
        }
    }

    if (i) {
        for (j = i; j < 4; j++) {
            char_array_4[j] = 0;
        }

        for (j = 0; j < 4; j++) {
            char_array_4[j] = GetCharIdx(char_array_4[j]) & 0xFF;
        }

        char_array_3[0] = ((char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4)) & 0xFF;
        char_array_3[1] = (((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2)) & 0xFF;
        char_array_3[2] = (((char_array_4[2] & 0x3) << 6) + char_array_4[3]) & 0xFF;

        for (j = 0; (j < i - 1); j++) {
            out.append(char_array_3[j]);
        }
    }
}

X_NAMESPACE_END
