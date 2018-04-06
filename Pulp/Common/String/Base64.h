#pragma once

#ifndef X_BASE64_H_
#define X_BASE64_H_

#include "StrRef.h"
#include <Containers\Array.h>

X_NAMESPACE_BEGIN(core)

class Base64
{
    static uint8 EncodingAlphabet[64];
    static uint8 DecodingAlphabet[256];

    static X_INLINE bool is_base64(uint8_t c)
    {
        return (strUtil::IsAlphaNum(c) || (c == '+') || (c == '/'));
    }

    X_NO_CREATE(Base64);

public:
    static core::string Encode(const core::string& str);
    static core::string Decode(const core::string& str);

    static core::string EncodeBytes(const uint8_t* pData, size_t len);
    static void DecodeBytes(core::string& str, core::Array<uint8_t>& out);
};

X_NAMESPACE_END

#endif // X_BASE64_H_