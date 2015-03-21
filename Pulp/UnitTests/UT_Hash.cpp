#include "stdafx.h"

#include "gtest/gtest.h"


#include <Hashing\crc32.h>
#include <Hashing\Fnva1Hash.h>
#include <Hashing\MurmurHash.h>
#include <Hashing\sha1.h>
#include <Hashing\MD5.h>
#include "Hashing\Adler32.h"

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

TEST(Hash, Adler32)
{
	Hash::Adler32Val val;

	val = Adler32("hello, can you fart in a cart?");

	EXPECT_EQ(val, 0xa3050a5e);
}