#include "stdafx.h"

#include "String\Base64.h"

X_USING_NAMESPACE;

using namespace core;

TEST(Base64, Encode)
{
    core::string encoded = Base64::Encode(core::string("my booty is firm and plump"));

    EXPECT_STREQ("bXkgYm9vdHkgaXMgZmlybSBhbmQgcGx1bXA=", encoded.c_str());
}

TEST(Base64, Decode)
{
    core::string decoded = Base64::Decode(core::string("bXkgYm9vdHkgaXMgZmlybSBhbmQgcGx1bXA="));

    EXPECT_STREQ("my booty is firm and plump", decoded.c_str());
}

TEST(Base64, Hex)
{
    uint8_t raw[17] = {0x81, 0xb9, 0xdc, 0xb1, 0x96, 0x87,
        0x89, 0xf9, 0xec, 0x29, 0x69, 0x38, 0xcf, 0x2d, 0xb8, 0x14, 0x92};

    core::string encoded = Base64::EncodeBytes(raw, sizeof(raw));
    core::Array<uint8_t> decoded(g_arena);

    Base64::DecodeBytes(encoded, decoded);

    EXPECT_STREQ("gbncsZaHifnsKWk4zy24FJI=", encoded.c_str());
    EXPECT_TRUE(decoded.size() == sizeof(raw) && 0 == std::memcmp(decoded.ptr(), raw, sizeof(raw)));
}