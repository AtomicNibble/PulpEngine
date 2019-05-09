#include "EngineCommon.h"
#include "Guid.h"

#include <rpc.h>

X_LINK_LIB("Rpcrt4.lib");

X_NAMESPACE_BEGIN(core)

namespace
{
    uint8_t hexDigitToChar(char ch)
    {
        // 0-9
        if (ch > 47 && ch < 58) {
            return ch - 48;
        }

        // a-f
        if (ch > 96 && ch < 103) {
            return ch - 87;
        }

        // A-F
        if (ch > 64 && ch < 71) {
            return ch - 55;
        }

        return 0;
    }

    bool isValidHexChar(char ch)
    {
        // 0-9
        if (ch > 47 && ch < 58) {
            return true;
        }

        // a-f
        if (ch > 96 && ch < 103) {
            return true;
        }

        // A-F
        if (ch > 64 && ch < 71) {
            return true;
        }

        return false;
    }

    // converts the two hexadecimal characters to an unsigned char (a byte)
    uint8_t hexPairToChar(char a, char b)
    {
        return hexDigitToChar(a) * 16 + hexDigitToChar(b);
    }

}

// create empty guid
Guid::Guid()
{
}

Guid::Guid(const GuidByteArr& bytes) :
    data_(bytes)
{
}

Guid::Guid(core::string_view str)
{
    char charOne = '\0';
    char charTwo = '\0';
    bool lookingForFirstChar = true;
    int32_t numByte = 0;

    for (const char &ch : str)
    {
        if (ch == '-') {
            continue;
        }

        if (numByte >= 16 || !isValidHexChar(ch))
        {
            // Invalid string so bail
            data_.bytes.fill(0);
            return;
        }

        if (lookingForFirstChar)
        {
            charOne = ch;
            lookingForFirstChar = false;
        }
        else
        {
            charTwo = ch;
            auto byte = hexPairToChar(charOne, charTwo);
            data_.bytes[numByte++] = byte;
            lookingForFirstChar = true;
        }
    }

    if (numByte < 16)
    {
        data_.bytes.fill(0);
        return;
    }
}

bool Guid::operator==(const Guid& oth) const
{
    return data_.bytes == oth.data_.bytes;
}

bool Guid::operator!=(const Guid& oth) const
{
    return !(*this == oth);
}

const Guid::GuidByteArr& Guid::bytes(void) const
{
    return data_.bytes;
}

const Guid::GUID& Guid::guid(void) const
{
    return data_.guid;
}

bool Guid::isValid(void) const
{
    Guid empty;
    return *this != empty;
}

const char* Guid::toString(GuidStr& buf) const
{
    auto& bytes = data_.bytes;

    buf.setFmt("%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x", 
        bytes[0],
        bytes[1],
        bytes[2],
        bytes[3],
        bytes[4],
        bytes[5],
        bytes[6],
        bytes[7],
        bytes[8],
        bytes[9],
        bytes[10],
        bytes[11],
        bytes[12],
        bytes[13],
        bytes[14],
        bytes[15]
    );

    return buf.c_str();
}

Guid Guid::newGuid(void)
{
    UUID newId;
    (void)UuidCreateSequential(&newId);

    Guid myGuid;
    myGuid.data_.bytes = {
        static_cast<uint8_t>((newId.Data1 >> 24) & 0xFF),
        static_cast<uint8_t>((newId.Data1 >> 16) & 0xFF),
        static_cast<uint8_t>((newId.Data1 >> 8) & 0xFF),
        static_cast<uint8_t>((newId.Data1) & 0xff),
        
        static_cast<uint8_t>((newId.Data2 >> 8) & 0xFF),
        static_cast<uint8_t>((newId.Data2) & 0xff),
        
        static_cast<uint8_t>((newId.Data3 >> 8) & 0xFF),
        static_cast<uint8_t>((newId.Data3) & 0xFF),

        newId.Data4[0],
        newId.Data4[1],
        newId.Data4[2],
        newId.Data4[3],
        newId.Data4[4],
        newId.Data4[5],
        newId.Data4[6],
        newId.Data4[7]
    };

    return myGuid;
}

X_NAMESPACE_END
