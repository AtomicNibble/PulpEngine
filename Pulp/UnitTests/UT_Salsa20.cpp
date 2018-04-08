#include "stdafx.h"

#include <Encryption\Salsa20.h>

X_USING_NAMESPACE;

using namespace core::Encryption;

namespace
{
    Salsa20::Key Key = {
        0xce, 0xab, 0x90, 0x52, 0xcc, 0x8e, 0xd8, 0x2e,
        0x02, 0x84, 0xaa, 0x6b, 0x91, 0x9b, 0x36, 0xcf,
        0x70, 0x78, 0xed, 0x26, 0xf4, 0x95, 0x86, 0x67,
        0x74, 0x99, 0x9b, 0x6f, 0xaf, 0x41, 0xb3, 0xb4};

    Salsa20::Iv Iv = {
        0x03, 0x20, 0x6a, 0x36, 0xbc, 0xc5, 0x3a, 0x4c};

    static const char sampleText[781] = "Contrary to popular belief, Lorem Ipsum is not"
                                        " simply random text. It has roots in a piece of classical Latin literature"
                                        " from 45 BC, making it over 2000 years old. Richard McClintock, a Latin professor"
                                        " at Hampden-Sydney College in Virginia, looked up one of the more obscure"
                                        " Latin words, consectetur, from a Lorem Ipsum passage, and going through"
                                        " the cites of the word in classical literature, discovered the"
                                        " undoubtable source. Lorem Ipsum comes from sections 1.10.32 and 1.10.33 of "
                                        "de Finibus Bonorum et Malorum\" (The Extremes of Good and Evil) by Cicero,"
                                        " written in 45 BC. This book is a treatise on the theory of ethics, very"
                                        " popular during the Renaissance. The first line of Lorem Ipsum, \"Lorem ipsum"
                                        " dolor sit amet..\", comes from a line in section 1.10.32. The standard chun";
} // namespace

TEST(Encryption, Salsa20)
{
    Salsa20 salsa20;

    salsa20.setKey(Key);
    salsa20.setIv(Iv);

    char textCpy[1024] = {0};
    strcpy_s(textCpy, sizeof(sampleText), sampleText);

    salsa20.processBytes(core::span<const char>(textCpy), core::span<char>(textCpy));

    // reset iv.
    salsa20.setIv(Iv);
    salsa20.processBytes(core::span<const char>(textCpy), core::span<char>(textCpy));

    EXPECT_STREQ(textCpy, sampleText);
}

TEST(Encrypt, Salsa20_multiBuf)
{
    const uint8_t TextIn[62] = "this is some thex that we arre going to encrypt with salsa20.";
    uint8_t EncryptedBuf[62];

    const Salsa20::Key key = {
        0x7B, 0xD9, 0x2B, 0x0E, 0x53, 0xF7, 0x44, 0xFA,
        0xDF, 0x03, 0x0E, 0x55, 0xB3, 0x15, 0xC7, 0x2B,
        0x5C, 0x1D, 0x70, 0x70, 0xF5, 0x35, 0x72, 0xAE,
        0xE6, 0xD7, 0x0A, 0x36, 0x92, 0x0A, 0xD9, 0x91};
    const Salsa20::Iv iv = {
        0x62, 0x28, 0x72, 0x6A,
        0x9F, 0x4F, 0x3B, 0x08};

    Salsa20 enc;

    enc.setKey(key);
    enc.setIv(iv);
    enc.processBytes(TextIn, EncryptedBuf);

    // expected.
    const uint8_t Expected[62] = {
        0xd5, 0x3e, 0xed, 0xd3, 0x7a, 0x78, 0x70, 0x90, 0xa3, 0xdd,
        0xc7, 0xde, 0xe3, 0xd9, 0x05, 0x93, 0x3a, 0xff, 0x9c, 0x7c,
        0x88, 0xcd, 0x91, 0x36, 0x68, 0x6c, 0xd5, 0x50, 0x02, 0xa2,
        0xf9, 0x02, 0x3d, 0x9a, 0xa4, 0xd1, 0x6a, 0xf8, 0x87, 0x2b,
        0x14, 0x30, 0x92, 0x66, 0x79, 0xdf, 0xee, 0xab, 0xdb, 0xe6,
        0xe4, 0x23, 0x7a, 0x01, 0x75, 0x9a, 0x56, 0x70, 0xc4, 0x9f,
        0x46, 0x9d};

    EXPECT_TRUE(0 == std::memcmp(EncryptedBuf, Expected, sizeof(EncryptedBuf)));
}

TEST(Encrypt, Salsa20_offset)
{
    const uint8_t hexData_enc[448] = {
        0xCD, 0xE5, 0x7D, 0x75, 0xE5, 0xB4, 0x93, 0xEA, 0x97, 0xC6, 0xD6, 0x3C, 0xE6, 0x0D, 0xD2, 0xBB,
        0x62, 0x82, 0x1E, 0x11, 0x1D, 0x2B, 0x4C, 0xC3, 0xBE, 0xA1, 0x59, 0x68, 0xC6, 0xF7, 0xE0, 0x60,
        0x1C, 0x07, 0xB1, 0x27, 0x81, 0xD2, 0x39, 0x9A, 0xC6, 0x3F, 0x12, 0xD3, 0x4F, 0x3C, 0xBC, 0x16,
        0xCD, 0x03, 0x46, 0xF2, 0xFE, 0xBB, 0x6C, 0xF1, 0xB0, 0x15, 0x9C, 0x97, 0x97, 0x6F, 0x7C, 0x43,
        0xDD, 0x67, 0xF7, 0x7E, 0x10, 0xAC, 0x50, 0x3C, 0x7F, 0xEE, 0x03, 0x18, 0xC6, 0x28, 0xD5, 0xC5,
        0x05, 0x33, 0x0E, 0x03, 0x1F, 0x23, 0xF2, 0x8A, 0xBC, 0xEA, 0xB2, 0x79, 0x98, 0x33, 0xD9, 0x02,
        0xEF, 0x05, 0x5E, 0x22, 0x8C, 0x1B, 0x68, 0xE6, 0xBF, 0x49, 0xED, 0x86, 0x96, 0x77, 0xDB, 0xCE,
        0xC4, 0x25, 0x4A, 0xEC, 0xC0, 0xC7, 0x49, 0x7A, 0xF4, 0xC1, 0x25, 0x71, 0x1B, 0x83, 0x01, 0x52,
        0xB5, 0x98, 0xFF, 0xCF, 0xCE, 0x96, 0x5B, 0x1F, 0x44, 0x3D, 0x1D, 0x40, 0xDD, 0x2D, 0xBC, 0x51,
        0xE3, 0x02, 0xC4, 0x2B, 0xE0, 0x7B, 0x3F, 0x9F, 0xE2, 0x51, 0x4B, 0xD2, 0x31, 0xE1, 0x88, 0x5B,
        0xC9, 0xB9, 0xE5, 0x43, 0xA0, 0xA3, 0xED, 0x31, 0x60, 0x78, 0x04, 0x56, 0x89, 0xA0, 0xA8, 0xCE,
        0x17, 0x7D, 0xCC, 0x85, 0x6E, 0x2E, 0xA9, 0x43, 0x36, 0x67, 0xB2, 0xC9, 0xC3, 0xAC, 0x13, 0xDE,
        0x67, 0xEF, 0xD9, 0x54, 0x41, 0x08, 0xCC, 0x12, 0xE3, 0xA9, 0x34, 0x41, 0xEF, 0x69, 0x96, 0x62,
        0x3B, 0xE6, 0x9C, 0x0F, 0xA1, 0x88, 0xC6, 0x60, 0xF2, 0x1D, 0x94, 0x8F, 0x52, 0x56, 0x2C, 0x9E,
        0xD8, 0xC4, 0x3E, 0x5B, 0x18, 0xEA, 0x5D, 0x3A, 0x33, 0x56, 0x79, 0xDE, 0xAD, 0x75, 0x5A, 0x49,
        0x5D, 0x0E, 0x8E, 0x71, 0xE2, 0x8A, 0x51, 0x6E, 0xEB, 0x4F, 0x29, 0x3E, 0x59, 0xA8, 0xB2, 0xE6,
        0xE0, 0x42, 0xC7, 0x60, 0xFE, 0xD7, 0xF8, 0x28, 0xBE, 0x86, 0x79, 0xF3, 0xB0, 0x4C, 0xAF, 0x91,
        0x48, 0x81, 0xD4, 0xEE, 0xA1, 0x39, 0x4E, 0xFF, 0x76, 0xF9, 0x82, 0x47, 0xEF, 0xCB, 0xB2, 0xE1,
        0x5C, 0xD6, 0xD9, 0x62, 0x46, 0xFA, 0x8F, 0x13, 0xAC, 0x22, 0x51, 0x4F, 0x82, 0xF6, 0x0E, 0x8A,
        0xDE, 0x62, 0xC1, 0x5D, 0x6E, 0xA2, 0x6C, 0x37, 0x12, 0x45, 0xAA, 0x51, 0x5D, 0xAA, 0x13, 0x4B,
        0x95, 0xD8, 0xA1, 0xB6, 0x1F, 0xDD, 0x32, 0x31, 0xD8, 0xF2, 0xE7, 0xEC, 0x0E, 0x6D, 0xD7, 0xD6,
        0x30, 0x88, 0xC2, 0xAF, 0x07, 0x1B, 0x7F, 0xE8, 0x4B, 0x69, 0x52, 0xAC, 0x5A, 0x8D, 0xAC, 0x43,
        0x26, 0x69, 0x47, 0x4C, 0x2C, 0xA9, 0x29, 0x1B, 0x3A, 0x2D, 0xA5, 0x7A, 0x6E, 0x2D, 0xC0, 0x5B,
        0x7B, 0xA1, 0x1E, 0x8D, 0xC6, 0xAF, 0x75, 0x4A, 0x89, 0xAB, 0x30, 0x2A, 0x4B, 0xF2, 0xBE, 0x12,
        0x3D, 0x0F, 0xEF, 0x08, 0x7F, 0x0C, 0x83, 0x6A, 0x72, 0x7D, 0xB6, 0x7B, 0xA0, 0x3A, 0x6D, 0x79,
        0x50, 0xB8, 0x0A, 0x2F, 0xD7, 0xC4, 0x01, 0x8D, 0x2F, 0x07, 0xA8, 0x10, 0x5B, 0x1B, 0x1E, 0x58,
        0xF7, 0xD9, 0x1D, 0x34, 0xA2, 0x76, 0xFD, 0x1F, 0x09, 0xA6, 0x02, 0xEA, 0x3D, 0x95, 0x55, 0xB7,
        0xBE, 0x5E, 0x5F, 0xD1, 0x8B, 0x8D, 0x3D, 0xCB, 0x0C, 0x60, 0x2F, 0x44, 0xA1, 0xD0, 0x9E, 0xB9};

    const uint8_t hexData_dec[448] = {
        0x02, 0x00, 0x00, 0x00, 0x23, 0x00, 0x00, 0x00, 0x42, 0x58, 0x4D, 0x4C, 0xEA, 0x03, 0x01, 0x00,
        0x9B, 0x00, 0x00, 0x00, 0x2C, 0x0A, 0x00, 0x00, 0xE0, 0x16, 0x00, 0x00, 0x1E, 0x06, 0x00, 0x00,
        0xC8, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x8E, 0x2A, 0x00, 0x00, 0x78, 0x9C, 0xB5, 0x9D,
        0x0F, 0x5C, 0x54, 0x55, 0xFA, 0xC6, 0x2F, 0x82, 0x4A, 0x46, 0x85, 0x8A, 0xA6, 0x46, 0x45, 0x65,
        0xC5, 0x96, 0x15, 0x25, 0x15, 0x1A, 0x0A, 0x22, 0x1A, 0x16, 0x2A, 0x01, 0x2A, 0xA9, 0x89, 0x08,
        0xA3, 0x90, 0x23, 0x20, 0x0C, 0x8A, 0xA6, 0x49, 0xE5, 0xEE, 0x62, 0xE1, 0xDF, 0xB0, 0xA8, 0xB4,
        0xA5, 0xCD, 0xCC, 0x8A, 0x92, 0x14, 0x5B, 0x2A, 0x56, 0xB1, 0x48, 0xAD, 0xA5, 0x5D, 0x7F, 0x9B,
        0x15, 0x15, 0x95, 0x96, 0x6D, 0x6E, 0x59, 0x4B, 0x65, 0x2D, 0x95, 0xE9, 0x6F, 0xCE, 0xCC, 0x73,
        0xBC, 0x8F, 0x34, 0x2F, 0x1D, 0x96, 0x8F, 0xF7, 0xF3, 0xC1, 0x19, 0xCF, 0xF7, 0x3D, 0xE7, 0x9E,
        0x73, 0x9E, 0xE7, 0x3D, 0xF7, 0xCE, 0xBD, 0x97, 0x21, 0x2E, 0x2F, 0xCF, 0x69, 0x4D, 0x48, 0x9D,
        0x98, 0x5A, 0x90, 0x91, 0x39, 0xDB, 0x8A, 0x77, 0xCC, 0xCC, 0x28, 0x72, 0xBA, 0x46, 0xE4, 0xCE,
        0x72, 0x3A, 0x46, 0x67, 0x38, 0x9D, 0x79, 0x33, 0x67, 0x5A, 0x71, 0x79, 0x45, 0xB9, 0x99, 0x8E,
        0xC4, 0x9C, 0x59, 0xD9, 0xAE, 0x91, 0x79, 0xCE, 0xBC, 0x02, 0xEB, 0xC6, 0x82, 0x9C, 0xAC, 0x64,
        0x47, 0x61, 0x9E, 0xB3, 0xC8, 0x95, 0x93, 0x97, 0x6B, 0xA5, 0xE4, 0x3B, 0x32, 0x8B, 0x9C, 0x19,
        0x05, 0xA3, 0x72, 0x33, 0x66, 0x38, 0x1D, 0x59, 0xD6, 0x98, 0x2C, 0x6B, 0x44, 0x61, 0xA1, 0xC3,
        0x65, 0x8D, 0xC9, 0xCD, 0x72, 0x14, 0x5B, 0xA3, 0x72, 0x5D, 0x39, 0xAE, 0x05, 0xD6, 0x84, 0x31,
        0x23, 0x8B, 0x0A, 0x5D, 0x79, 0x73, 0x72, 0x16, 0x3A, 0x26, 0x3A, 0xB2, 0x73, 0x32, 0x9D, 0x8E,
        0xC4, 0xBC, 0xCC, 0x0C, 0x4F, 0x03, 0xF1, 0x0B, 0x72, 0x33, 0xE6, 0xE4, 0x64, 0xA6, 0x64, 0x67,
        0x64, 0xE5, 0xCD, 0xD7, 0xAD, 0x8C, 0xCD, 0xC8, 0xF7, 0x16, 0x58, 0xC9, 0x29, 0xA9, 0xEE, 0x3E,
        0x16, 0x8E, 0x4B, 0x49, 0x4F, 0x4E, 0x89, 0xCF, 0x70, 0x65, 0xA8, 0x37, 0x19, 0xB9, 0xB3, 0x1C,
        0x59, 0xA3, 0x9D, 0x79, 0x19, 0x2E, 0x6F, 0x99, 0x35, 0xD1, 0xDD, 0x9B, 0x39, 0x8E, 0x93, 0x9B,
        0x48, 0x29, 0xCA, 0xA5, 0x5E, 0x4F, 0x18, 0x93, 0x8E, 0x1D, 0xA7, 0xC7, 0xE5, 0x15, 0x5B, 0x1E,
        0x32, 0x62, 0x61, 0xCE, 0x9C, 0x22, 0x57, 0xB6, 0x35, 0x36, 0x2D, 0x6C, 0x6C, 0x4E, 0x6E, 0x8E,
        0xAF, 0x5D, 0x79, 0x86, 0x82, 0x9D, 0xF8, 0xC0, 0x63, 0x33, 0x5C, 0x05, 0x39, 0xC5, 0xE0, 0xA3,
        0x72, 0x8B, 0xE6, 0x4C, 0xCC, 0x70, 0x16, 0x39, 0x7C, 0x46, 0xE6, 0x65, 0x39, 0x9C, 0x14, 0x68,
        0xE1, 0xFD, 0xE8, 0x82, 0xBC, 0x5C, 0x97, 0xDD, 0xCF, 0x31, 0xB9, 0x2E, 0x47, 0x6E, 0xA1, 0x9A,
        0x31, 0x1F, 0x6D, 0xB8, 0xA1, 0xDC, 0x95, 0x84, 0xD1, 0xA9, 0x8E, 0x82, 0x82, 0x8C, 0x9C, 0x5C,
        0x84, 0xB4, 0xD5, 0x8D, 0x5A, 0x76, 0xCC, 0x74, 0x3A, 0x32, 0xD5, 0xDC, 0x4F, 0x76, 0x14, 0xE4};

    const Salsa20::Iv iv = {0x64, 0x1a, 0x9a, 0x06, 0x48, 0xaa, 0xb2, 0x93};
    const Salsa20::Key key = {0xc0, 0x26, 0x92, 0xf8, 0xee, 0xfc, 0xbd, 0x0b, 0x5d, 0x94,
        0xcd, 0x5f, 0x84, 0x12, 0xbd, 0x27, 0xb9, 0x61, 0x5c, 0xb4, 0xe2,
        0x9d, 0x42, 0xbf, 0x03, 0x64, 0xc0, 0xed, 0xfa, 0x01, 0x67, 0xb6};

    Salsa20 salsa20;

    salsa20.setKey(key);
    salsa20.setIv(iv);

    uint8_t out[sizeof(hexData_enc)] = {0};

    salsa20.processBytes(core::span<const uint8_t>(hexData_enc), core::span<uint8_t>(out), 0x110);

    EXPECT_EQ(0, std::memcmp(out, hexData_dec, sizeof(hexData_enc)));
}