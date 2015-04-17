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