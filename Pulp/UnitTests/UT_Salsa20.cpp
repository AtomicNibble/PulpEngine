#include "stdafx.h"
#include "gtest/gtest.h"

#include <Encryption\Salsa20.h>

X_USING_NAMESPACE;

using namespace core::Encryption;

namespace
{
	static uint8_t Key[Salsa20::KEY_SIZE] = {
		0xce, 0xab, 0x90, 0x52, 0xcc, 0x8e, 0xd8, 0x2e,
		0x02, 0x84, 0xaa, 0x6b, 0x91, 0x9b, 0x36, 0xcf,
		0x70, 0x78, 0xed, 0x26, 0xf4, 0x95, 0x86, 0x67, 
		0x74, 0x99, 0x9b, 0x6f, 0xaf, 0x41, 0xb3, 0xb4
	};

	static uint8_t Iv[Salsa20::IV_SIZE] = {
		0x03, 0x20, 0x6a, 0x36, 0xbc, 0xc5, 0x3a, 0x4c
	};

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
}

TEST(Encryption, Salsa20)
{
	Salsa20 salsa20;

	salsa20.setKey(Key);
	salsa20.setIv(Iv);

	char textCpy[1024] = { 0 };
	strcpy_s(textCpy, sizeof(sampleText), sampleText);

	salsa20.processBytes(reinterpret_cast<uint8_t*>(textCpy),
		reinterpret_cast<uint8_t*>(textCpy), sizeof(textCpy));

	// reset iv.
	salsa20.setIv(Iv);

	salsa20.processBytes(reinterpret_cast<uint8_t*>(textCpy),
		reinterpret_cast<uint8_t*>(textCpy), sizeof(textCpy));

	EXPECT_STREQ(textCpy, sampleText);
}



TEST(Encrypt, Salsa20_multiBuf)
{
	const uint8_t TextIn[62] = "this is some thex that we arre going to encrypt with salsa20.";
	uint8_t EncryptedBuf[62];

	Salsa20::Key key = {
		0x7B, 0xD9, 0x2B, 0x0E, 0x53, 0xF7, 0x44, 0xFA,
		0xDF, 0x03, 0x0E, 0x55, 0xB3, 0x15, 0xC7, 0x2B,
		0x5C, 0x1D, 0x70, 0x70, 0xF5, 0x35, 0x72, 0xAE,
		0xE6, 0xD7, 0x0A, 0x36, 0x92, 0x0A, 0xD9, 0x91
	};
	Salsa20::Iv iv = {
		0x62, 0x28, 0x72, 0x6A,
		0x9F, 0x4F, 0x3B, 0x08
	};

	Salsa20 enc;

	enc.setKey(key);
	enc.setIv(iv);
	enc.processBytes(TextIn, EncryptedBuf, 62);

	// expected.
	const uint8_t Expected[62] = {
		0xfe, 0xa8, 0xf2, 0xf5, 0x25, 0x46, 0x60, 0xc9,
		0xc8, 0x1c, 0xe0, 0x33, 0xc6, 0x61, 0x5f, 0xc9,
		0xb5, 0xb0, 0xfc, 0x67, 0xe9, 0x9f, 0x67, 0x94,
		0x72, 0x04, 0x69, 0x53, 0x90, 0xfe, 0x95, 0x3e,
		0xd4, 0x3f, 0x20, 0x9f, 0x7f, 0x67, 0x7c, 0x88,
		0x91, 0x3f, 0xa0, 0x32, 0xf3, 0xfb, 0x6f, 0xae,
		0x48, 0x0e, 0x51, 0xe5, 0xd1, 0xd8, 0x1f, 0x44,
		0x7f, 0x3d, 0xef, 0x5a, 0xb5, 0x47 };

	EXPECT_TRUE(0 == std::memcmp(EncryptedBuf, Expected, sizeof(EncryptedBuf)));
}