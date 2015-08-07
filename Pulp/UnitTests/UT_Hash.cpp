#include "stdafx.h"

#include "gtest/gtest.h"


#include <Hashing\crc32.h>
#include <Hashing\Fnva1Hash.h>
#include <Hashing\MurmurHash.h>
#include <Hashing\sha1.h>
#include <Hashing\sha512.h>
#include <Hashing\MD5.h>
#include "Hashing\Adler32.h"

#include <Util\EndianUtil.h>

X_USING_NAMESPACE;

using namespace core;
using namespace core::Hash;


TEST(Hash, crc32) {

	Crc32 crc;

	uint32 val = crc.GetCRC32("my name is wincat");
	uint32 val2 = crc.GetCRC32("my name is Wincat");

	EXPECT_EQ(0x4b2da119, val);
	EXPECT_EQ(0x4c81a42f, val2);

	EXPECT_FALSE(crc.GetCRC32("hello") == crc.GetCRC32("HELLO"));
	EXPECT_TRUE(crc.GetCRC32("hello") == crc.GetCRC32Lowercase("HELLO"));

}

TEST(Hash, Fnva) {

	uint32 hash = Fnv1aHash("readme.txt", 10);
	uint64 hash64 = Int64::Fnv1aHash("readme.txt", 10);

	EXPECT_TRUE(hash == 0x966dcb41);
	EXPECT_TRUE(hash64 == 0xdaca8649b3b4bd97);

	EXPECT_FALSE(hash == Fnv1aHash("readme.txt.", 11));
}

TEST(Hash, Murmur) {


	uint32 val = 0x131;
	uint32 val2 = 0x132;

	uint32 hash = Hash::MurmurHash2(&val, sizeof(val), 0);
	uint32 hash2 = Hash::MurmurHash2(&val2, sizeof(val2), 0);


	EXPECT_EQ(hash, 0xe6e00531);
	EXPECT_EQ(hash2, 0x87c86240);
	EXPECT_NE(hash,hash2);
}


TEST(Hash, MD5)
{
	Hash::MD5Digest val, expected;
	Hash::MD5Digest::String str;

	expected.data[0] = 0x0307ed05;
	expected.data[1] = 0xbb095b54;
	expected.data[2] = 0xc60e9c12;
	expected.data[3] = 0xcf921e78;

	Hash::MD5 md5;
	md5.Init();
	md5.update("tickle my pickle");
	val = md5.finalize();

	val.ToString(str);

	EXPECT_EQ(expected, val);
	EXPECT_STREQ("05ed0703545b09bb129c0ec6781e92cf", str);
}

TEST(Hash, SHA1)
{
	Hash::SHA1Digest val, expected;
	Hash::SHA1Digest::String str;

	expected.data[0] = 0x2ed5f43d;
	expected.data[1] = 0x5d38c6d7;
	expected.data[2] = 0xe329f2aa;
	expected.data[3] = 0x3f04d422;
	expected.data[4] = 0xe25e92fe;

	Hash::SHA1 sha1;
	sha1.Init();
	sha1.update("hash me baby!");
	val = sha1.finalize();

	val.ToString(str);

	EXPECT_EQ(expected, val);
	EXPECT_STREQ("3df4d52ed7c6385daaf229e322d4043ffe925ee2", str);
}

TEST(Hash, SHA512_blank)
{
	Hash::SHA512Digest val, expected;
	Hash::SHA512Digest::String str;

	expected.data[0] = Endian::swap(0xcf83e1357eefb8bd);
	expected.data[1] = Endian::swap(0xf1542850d66d8007);
	expected.data[2] = Endian::swap(0xd620e4050b5715dc);
	expected.data[3] = Endian::swap(0x83f4a921d36ce9ce);
	expected.data[4] = Endian::swap(0x47d0d13c5d85f2b0);
	expected.data[5] = Endian::swap(0xff8318d2877eec2f);
	expected.data[6] = Endian::swap(0x63b931bd47417a81);
	expected.data[7] = Endian::swap(0xa538327af927da3e);

	Hash::SHA512 sha512;
	sha512.Init();
	sha512.update("");
	val = sha512.finalize();

	val.ToString(str);

	EXPECT_EQ(expected, val);
	EXPECT_STREQ("cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a9"
		"21d36ce9ce47d0d13c5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e", str);
}

TEST(Hash, SHA512_abc)
{
	Hash::SHA512Digest val, expected;
	Hash::SHA512Digest::String str;

	expected.data[0] = Endian::swap(0xddaf35a193617aba);
	expected.data[1] = Endian::swap(0xcc417349ae204131);
	expected.data[2] = Endian::swap(0x12e6fa4e89a97ea2);
	expected.data[3] = Endian::swap(0x0a9eeee64b55d39a);
	expected.data[4] = Endian::swap(0x2192992a274fc1a8);
	expected.data[5] = Endian::swap(0x36ba3c23a3feebbd);
	expected.data[6] = Endian::swap(0x454d4423643ce80e);
	expected.data[7] = Endian::swap(0x2a9ac94fa54ca49f);

	Hash::SHA512 sha512;
	sha512.Init();
	sha512.update("abc");
	val = sha512.finalize();

	val.ToString(str);

	EXPECT_EQ(expected, val);
	EXPECT_STREQ("ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b5"
		"5d39a2192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f", str);
}

TEST(Hash, SHA512_896)
{
	Hash::SHA512Digest val, expected;
	Hash::SHA512Digest::String str;

	expected.data[0] = Endian::swap(0x8e959b75dae313da);
	expected.data[1] = Endian::swap(0x8cf4f72814fc143f);
	expected.data[2] = Endian::swap(0x8f7779c6eb9f7fa1);
	expected.data[3] = Endian::swap(0x7299aeadb6889018);
	expected.data[4] = Endian::swap(0x501d289e4900f7e4);
	expected.data[5] = Endian::swap(0x331b99dec4b5433a);
	expected.data[6] = Endian::swap(0xc7d329eeb6dd2654);
	expected.data[7] = Endian::swap(0x5e96e55b874be909);

	Hash::SHA512 sha512;
	sha512.Init();
	sha512.update("abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhi"
		"jklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu");
	val = sha512.finalize();

	val.ToString(str);

	EXPECT_EQ(expected, val);
	EXPECT_STREQ("8e959b75dae313da8cf4f72814fc143f8f7779c6eb9f7fa17299aeadb688901"
		"8501d289e4900f7e4331b99dec4b5433ac7d329eeb6dd26545e96e55b874be909", str);
}



TEST(Hash, Adler32)
{
	Hash::Adler32Val val;

	val = Adler32("hello, can you fart in a cart?");

	EXPECT_EQ(val, 0xa3050a5e);
}