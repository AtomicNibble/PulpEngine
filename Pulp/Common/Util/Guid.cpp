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
Guid::Guid() : 
    bytes_{ {0} }
{ 
}

Guid::Guid(const GuidByteArr& bytes) :
    bytes_(bytes)
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
            bytes_.fill(0);
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
            bytes_[numByte++] = byte;
            lookingForFirstChar = true;
        }
    }

    if (numByte < 16)
    {
        bytes_.fill(0);
        return;
    }
}

bool Guid::operator==(const Guid& oth) const
{
    return bytes_ == oth.bytes_;
}

bool Guid::operator!=(const Guid& oth) const
{
    return !(*this == oth);
}

const Guid::GuidByteArr& Guid::bytes(void) const
{
    return bytes_;
}

bool Guid::isValid(void) const
{
    Guid empty;
    return *this != empty;
}

const char* Guid::toString(GuidStr& buf) const
{
    buf.setFmt("%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x", 
        bytes_[0],
        bytes_[1],
        bytes_[2],
        bytes_[3],
        bytes_[4],
        bytes_[5],
        bytes_[6],
        bytes_[7],
        bytes_[8],
        bytes_[9],
        bytes_[10],
        bytes_[11],
        bytes_[12],
        bytes_[13],
        bytes_[14],
        bytes_[15]
    );

    return buf.c_str();
}

Guid Guid::newGuid(void)
{
    UUID newId;
    (void)UuidCreateSequential(&newId);

    Guid myGuid;
    myGuid.bytes_ = {
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